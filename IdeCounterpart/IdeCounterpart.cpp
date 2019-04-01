#define IUdpConnectorService_EXPORTS

#include "VersionInfo.h"
#include "IdeCounterpart.h"
#include "EnumStringConvertor.h"
#include "UdpMessage.h"
#include "crc.h"
#include "Trace.h"
#include "rapidjson/pointer.h"

#include "iqrf__IdeCounterpart.hxx"

TRC_INIT_MODULE(iqrf::IdeCounterpart);

namespace iqrf {

  IdeCounterpart::IdeCounterpart()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  IdeCounterpart::~IdeCounterpart()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  void IdeCounterpart::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IdeCounterpart instance activate" << std::endl <<
      "******************************"
    );

    using namespace rapidjson;
    const Document& doc = props->getAsJson();

    {
      const Value* val = rapidjson::Pointer("/gwIdentModeByte").Get(doc);
      if (val && val->IsInt()) {
        m_gwIdentModeByte = (uint8_t)val->GetInt();
      }
    }

    {
      const Value* val = rapidjson::Pointer("/gwIdentName").Get(doc);
      if (val && val->IsString()) {
        m_gwIdentName = val->GetString();
      }
    }

    {
      const Value* val = rapidjson::Pointer("/gwIdentIpStack").Get(doc);
      if (val && val->IsString()) {
        m_gwIdentIpStack = val->GetString();
      }
    }

    {
      const Value* val = rapidjson::Pointer("/gwIdentNetBios").Get(doc);
      if (val && val->IsString()) {
        m_gwIdentNetBios = val->GetString();
      }
    }

    {
      const Value* val = rapidjson::Pointer("/gwIdentPublicIp").Get(doc);
      if (val && val->IsString()) {
        m_gwIdentPublicIp = val->GetString();
      }
    }

    m_messaging->registerMessageHandler([&](const std::string& messagingId, const std::vector<uint8_t>& msg)
    {
      (void)messagingId;  //silence -Wunused-parameter
      return handleMessageFromUdp(msg);
    });

    setMode(IdeCounterpart::Mode::Operational);

    TRC_FUNCTION_LEAVE("")
  }

  void IdeCounterpart::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "IdeCounterpart instance deactivate" << std::endl <<
      "******************************"
    );

    setMode(IdeCounterpart::Mode::Operational);
    
    m_messaging->unregisterMessageHandler();

    TRC_FUNCTION_LEAVE("")
  }

  void IdeCounterpart::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void IdeCounterpart::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IdeCounterpart::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void IdeCounterpart::attachInterface(iqrf::IUdpMessagingService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_messaging = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void IdeCounterpart::detachInterface(iqrf::IUdpMessagingService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_messaging == iface) {
      m_messaging = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void IdeCounterpart::attachInterface(iqrf::IIqrfChannelService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_iqrfChannelService = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void IdeCounterpart::detachInterface(iqrf::IIqrfChannelService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_iqrfChannelService == iface) {
      m_iqrfChannelService = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void IdeCounterpart::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_iqrfDpaService = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void IdeCounterpart::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_iqrfDpaService == iface) {
      m_iqrfDpaService = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void IdeCounterpart::setMode(Mode mode)
  {
    TRC_FUNCTION_ENTER(NAME_PAR(mode, (int)mode));

    std::lock_guard<std::mutex> lck(m_modeMtx);

    switch (mode) {

    case Mode::Operational:
      m_exclusiveAcessor.reset();
      m_snifferAcessor.reset();
      m_mode = mode;
      break;

    case Mode::Forwarding:
      m_exclusiveAcessor.reset();
      m_snifferAcessor = m_iqrfChannelService->getAccess([&](const std::basic_string<unsigned char>& received)->int {
        return sendMessageToIde(received);
      }, IIqrfChannelService::AccesType::Sniffer);
      m_mode = mode;
      break;

    case Mode::Service:
    {
      m_snifferAcessor.reset();
      m_exclusiveAcessor = m_iqrfChannelService->getAccess([&](const std::basic_string<unsigned char>& received)->int {
        return sendMessageToIde(received);
      }, IIqrfChannelService::AccesType::Exclusive);
      m_mode = mode;
      break;
    }
    break;

    default:;
    }

    TRC_INFORMATION("Set mode " << ModeStringConvertor::enum2str(m_mode));

    TRC_FUNCTION_LEAVE("");
  }

  int IdeCounterpart::sendMessageToIde(const std::basic_string<unsigned char>& message)
  {
    std::basic_string<unsigned char> udpMessage(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
    udpMessage[cmd] = (unsigned char)IQRF_UDP_IQRF_SPI_DATA;
    encodeMessageUdp(udpMessage, message);
    m_messaging->sendMessage("", udpMessage);
    return 0;
  }

  void IdeCounterpart::getGwIdent(std::basic_string<unsigned char>& message)
  {
    //1. - GW type e.g.: GW - ETH - 02A
    //2. - FW version e.g. : 2.50
    //3. - MAC address e.g. : 00 11 22 33 44 55
    //4. - TCP / IP Stack version e.g. : 5.42
    //5. - IP address of GW e.g. : 192.168.2.100
    //6. - Net BIOS Name e.g. : iqrf_xxxx (15 characters)
    //7. - IQRF module OS version e.g. : 3.06D
    //8. - Public IP address e.g. : 213.214.215.120

    const char* crlf = "\x0D\x0A";

    std::basic_ostringstream<char> ostring;
    ostring << crlf <<
      m_gwIdentName << crlf <<
      DAEMON_VERSION << crlf <<
      m_messaging->getListeningMacAddress() << crlf <<
      m_gwIdentIpStack << crlf <<
      m_messaging->getListeningIpAddress() << crlf <<
      m_gwIdentNetBios << crlf <<
      m_iqrfDpaService->getCoordinatorParameters().osVersion << "(" << m_iqrfDpaService->getCoordinatorParameters().osBuild << ")" << crlf <<
      m_gwIdentPublicIp << crlf;

    std::string resp = ostring.str();
    std::basic_string<unsigned char> res((unsigned char*)resp.data(), resp.size());
    message = res;
  }

  void IdeCounterpart::getGwStatus(std::basic_string<unsigned char>& message)
  {
    // current date/time based on current system
    time_t now = time(0);
    tm *ltm = localtime(&now);

    message.resize(UdpGwStatus::unused12 + 1, '\0');
    //TODO get channel status to Channel iface
    message[trStatus] = 0x80;   //SPI_IQRF_SPI_READY_COMM = 0x80, see spi_iqrf.h
    message[supplyExt] = 0x01;  //DB3 0x01 supplied from external source
    message[timeSec] = (unsigned char)ltm->tm_sec;    //DB4 GW time – seconds(see Time and date coding)
    message[timeMin] = (unsigned char)ltm->tm_min;    //DB5 GW time – minutes
    message[timeHour] = (unsigned char)ltm->tm_hour;    //DB6 GW time – hours
    message[timeWday] = (unsigned char)ltm->tm_wday;    //DB7 GW date – day of the week
    message[timeMday] = (unsigned char)ltm->tm_mday;    //DB8 GW date – day
    message[timeMon] = (unsigned char)ltm->tm_mon;    //DB9 GW date – month
    message[timeYear] = (unsigned char)(ltm->tm_year % 100);   //DB10 GW date – year
  }

  int IdeCounterpart::handleMessageFromUdp(const std::vector<uint8_t>& msg)
  {
    //TRC_DBG("==================================" << std::endl <<
    //  "Received from UDP: " << std::endl << FORM_HEX(udpMessage.data(), udpMessage.size()));

    std::basic_string<uint8_t> udpMessage(msg.data(), msg.size());

    //size_t msgSize = udpMessage.size();
    std::basic_string<unsigned char> message;

    try {
      decodeMessageUdp(udpMessage, message);
    }
    catch (std::logic_error& e) {
      CATCH_EXC_TRC_WAR(std::logic_error, e, "wrong message");
      return -1;
    }

    switch (udpMessage[cmd])
    {
    case IQRF_UDP_GET_GW_INFO:          // --- Returns GW identification ---
    {
      ustring udpResponse(udpMessage);
      udpResponse[cmd] = udpResponse[cmd] | 0x80;
      ustring msg;
      getGwIdent(msg);
      encodeMessageUdp(udpResponse, msg);
      m_messaging->sendMessage("", udpResponse);
    }
    return 0;

    case IQRF_UDP_GET_GW_STATUS:          // --- Returns GW status ---
    {
      ustring udpResponse(udpMessage);
      udpResponse[cmd] = udpResponse[cmd] | 0x80;
      ustring msg;
      getGwStatus(msg);
      encodeMessageUdp(udpResponse, msg);
      m_messaging->sendMessage("", udpResponse);
    }
    return 0;

    case IQRF_UDP_WRITE_IQRF:       // --- Writes data to the TR module ---
    {
      //send response
      ustring udpResponse(udpMessage.substr(0, IQRF_UDP_HEADER_SIZE));
      udpResponse[cmd] = udpResponse[cmd] | 0x80;
      //TODO it is required to send back via subcmd write result - implement sync write with appropriate ret code
      if (m_exclusiveAcessor) { // exclusiveAccess
        udpResponse[subcmd] = IQRF_UDP_ACK;
      }
      else {
        udpResponse[subcmd] = IQRF_UDP_GW_NOT_SERVICE;
        TRC_WARNING(std::endl <<
          "****************************************************" << std::endl <<
          "CANNOT SEND DPA MESSAGE IN OPERATIONAL MODE" << std::endl <<
          "****************************************************" << std::endl <<
          "Messages from UDP are accepted only in service mode" << std::endl);
      }

      encodeMessageUdp(udpResponse);
      m_messaging->sendMessage("", udpResponse);

      if (m_exclusiveAcessor) { // exclusiveAccess
        m_exclusiveAcessor->send(message);
      }
    }
    return 0;

    default:
      //not implemented command
      std::basic_string<unsigned char> udpResponse(udpMessage);
      udpResponse[cmd] = udpResponse[cmd] | 0x80;
      udpResponse[subcmd] = (unsigned char)IQRF_UDP_NAK;
      encodeMessageUdp(udpResponse);
      m_messaging->sendMessage("", udpResponse);
      break;
    }
    return -1;
  }

  void IdeCounterpart::encodeMessageUdp(std::basic_string<unsigned char>& udpMessage, const std::basic_string<unsigned char>& message)
  {
    unsigned short dlen = (unsigned short)message.size();

    udpMessage.resize(IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE, '\0');
    udpMessage[gwAddr] = m_gwIdentModeByte; //IQRF_UDP_GW_ADR;
    udpMessage[dlen_H] = (unsigned char)((dlen >> 8) & 0xFF);
    udpMessage[dlen_L] = (unsigned char)(dlen & 0xFF);

    if (0 < dlen)
      udpMessage.insert(IQRF_UDP_HEADER_SIZE, message);

    uint16_t crc = Crc::get().GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE);
    udpMessage[dlen + IQRF_UDP_HEADER_SIZE] = (unsigned char)((crc >> 8) & 0xFF);
    udpMessage[dlen + IQRF_UDP_HEADER_SIZE + 1] = (unsigned char)(crc & 0xFF);
  }

  void IdeCounterpart::decodeMessageUdp(const std::basic_string<unsigned char>& udpMessage, std::basic_string<unsigned char>& message)
  {
    unsigned short dlen = 0;

    // Min. packet length check
    if (udpMessage.size() < IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE)
      THROW_EXC_TRC_WAR(std::logic_error, "Message is too short: " << MEM_HEX_CHAR(udpMessage.data(), udpMessage.size()));

    // GW_ADR check
    if (udpMessage[gwAddr] != m_gwIdentModeByte) //IQRF_UDP_GW_ADR)
      THROW_EXC_TRC_WAR(std::logic_error, "Message is has wrong GW_ADDR: " << PAR_HEX(udpMessage[gwAddr]));

    //iqrf data length
    dlen = (udpMessage[dlen_H] << 8) + udpMessage[dlen_L];

    // Max. packet length check
    if (static_cast<unsigned>((dlen + IQRF_UDP_HEADER_SIZE + IQRF_UDP_CRC_SIZE)) > IQRF_UDP_BUFFER_SIZE)
      THROW_EXC_TRC_WAR(std::logic_error, "Message is too long: " << PAR(dlen));

    // CRC check
    unsigned short crc = (udpMessage[IQRF_UDP_HEADER_SIZE + dlen] << 8) + udpMessage[IQRF_UDP_HEADER_SIZE + dlen + 1];
    if (crc != Crc::get().GetCRC_CCITT((unsigned char*)udpMessage.data(), dlen + IQRF_UDP_HEADER_SIZE))
      THROW_EXC_TRC_WAR(std::logic_error, "Message has wrong CRC");

    message = udpMessage.substr(IQRF_UDP_HEADER_SIZE, dlen);
  }



}
