/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define IReadTrConfService_EXPORTS

#include "ReadTrConfService.h"
#include "Trace.h"
#include "ComIqmeshNetworkReadTrConf.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "iqrf__ReadTrConfService.hxx"
#include <list>
#include <memory>
#include <math.h>
#include <bitset>

TRC_INIT_MODULE(iqrf::ReadTrConfService)

using namespace rapidjson;

namespace
{
  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;

  // Length of the configuration part
  static const uint8_t CONFIGURATION_LEN = 31;

  // Supported baud rates
  static uint32_t BaudRates[] =
  {
    1200,
    2400,
    4800,
    9600,
    19200,
    38400,
    57600,
    115200,
    230400
  };
}

namespace iqrf {

  // Holds information about result of read Tr configuration
  class ReadTrConfigResult
  {
  private:
    TPerOSReadCfg_Response m_hwpConfig;
    TEnumPeripheralsAnswer m_enumPer;
    // Status
    int m_status = 0;
    std::string m_statusStr = "ok";

    // Transaction results
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;

  public:
    // Status
    int getStatus() const { return m_status; };
    std::string getStatusStr() const { return m_statusStr; };
    void setStatus(const int status) {
      m_status = status;
    }
    void setStatus(const int status, const std::string statusStr) {
      m_status = status;
      m_statusStr = statusStr;
    }
    void setStatusStr(const std::string statusStr) {
      m_statusStr = statusStr;
    }

    TPerOSReadCfg_Response getHwpConfig() const {
      return m_hwpConfig;
    }
    void setHwpConfig(TPerOSReadCfg_Response hwpConfig) {
      m_hwpConfig = hwpConfig;
    }

    TEnumPeripheralsAnswer getEnumPer() const
    {
      return m_enumPer;
    }
    void setEnumPer(TEnumPeripheralsAnswer enumPer)
    {
      m_enumPer = enumPer;
    }

    // Adds transaction result into the list of results
    void addTransactionResult(std::unique_ptr<IDpaTransactionResult2>& transResult) {
      if (transResult != nullptr)
        m_transResults.push_back(std::move(transResult));
    }

    bool isNextTransactionResult() {
      return (m_transResults.size() > 0);
    }

    // Consumes the first element in the transaction results list
    std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult() {
      std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
      std::unique_ptr<IDpaTransactionResult2> tranResult = std::move(*iter);
      m_transResults.pop_front();
      return tranResult;
    }
  };

  // Implementation class
  class ReadTrConfService::Imp {
  private:
    // Parent object
    ReadTrConfService& m_parent;

    // Message type: IQMESH Network Read TR Configuration
    const std::string m_mTypeName_iqmeshNetworkReadTrConf = "iqmeshNetwork_ReadTrConf";
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    const MessagingInstance* m_messaging = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComIqmeshNetworkReadTrConf* m_comReadTrConf = nullptr;

    // Service input parameters
    TReadTrConfInputParams m_readTrConfParams;

  public:
    explicit Imp(ReadTrConfService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

    //---------------------------
    // Get peripheral information
    //---------------------------
    void getPerInfo(ReadTrConfigResult& readTrConfigResult, const uint16_t deviceAddress)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage getPerInfoRequest;
        DpaMessage::DpaPacket_t getPerInfoPacket;
        getPerInfoPacket.DpaRequestPacket_t.NADR = deviceAddress;
        getPerInfoPacket.DpaRequestPacket_t.PNUM = PNUM_ENUMERATION;
        getPerInfoPacket.DpaRequestPacket_t.PCMD = CMD_GET_PER_INFO;
        getPerInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        getPerInfoRequest.DataToBuffer(getPerInfoPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(getPerInfoRequest, transResult, m_readTrConfParams.repeat);
        TRC_DEBUG("Result from PNUM_ENUMERATION as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Device PNUM_ENUMERATION successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, getPerInfoRequest.PeripheralType())
          << NAME_PAR(Node address, getPerInfoRequest.NodeAddress())
          << NAME_PAR(Command, (int)getPerInfoRequest.PeripheralCommand())
        );
        TEnumPeripheralsAnswer enumPerAnswer = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer;
        readTrConfigResult.setEnumPer(enumPerAnswer);
        readTrConfigResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        readTrConfigResult.setStatus(transResult->getErrorCode(), e.what());
        readTrConfigResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //--------------------------------
    // Reads configuration of one node
    //--------------------------------
    void readConfig(ReadTrConfigResult& readTrConfigResult, const uint16_t deviceAddr, const uint16_t hwpId)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare the DPA request
        DpaMessage readHwpRequest;
        DpaMessage::DpaPacket_t readHwpPacket;
        readHwpPacket.DpaRequestPacket_t.NADR = deviceAddr;
        readHwpPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        readHwpPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ_CFG;
        readHwpPacket.DpaRequestPacket_t.HWPID = hwpId;
        readHwpRequest.DataToBuffer(readHwpPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(readHwpRequest, transResult, m_readTrConfParams.repeat);
        TRC_DEBUG("Result from CMD_OS_READ_CFG as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Device CMD_OS_READ_CFG successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, readHwpRequest.PeripheralType())
          << NAME_PAR(Node address, readHwpRequest.NodeAddress())
          << NAME_PAR(Command, (int)readHwpRequest.PeripheralCommand())
        );
        TPerOSReadCfg_Response hwpConfig = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSReadCfg_Response;
        readTrConfigResult.setHwpConfig(hwpConfig);
        readTrConfigResult.addTransactionResult(transResult);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        readTrConfigResult.setStatus(transResult->getErrorCode(), e.what());
        readTrConfigResult.addTransactionResult(transResult);
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(const int status, const std::string statusStr)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comReadTrConf->getMsgId());

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, statusStr);

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messaging, std::move(response));
    }

    //--------------------------
    // Creates and send response
    //--------------------------
    void createResponse(ReadTrConfigResult& readTrConfigResult)
    {
      Document response;

      // Set common parameters
      Pointer("/mType").Set(response, m_msgType->m_type);
      Pointer("/data/msgId").Set(response, m_comReadTrConf->getMsgId());

      // Set status
      int status = readTrConfigResult.getStatus();
      if (status == 0)
      {
        // Only one node - for the present time
        Pointer("/data/rsp/deviceAddr").Set(response, m_readTrConfParams.deviceAddress);

        // osRead object
        TPerOSReadCfg_Response hwpConfig = readTrConfigResult.getHwpConfig();

        // Get DPA version
        uint16_t dpaVer = readTrConfigResult.getEnumPer().DpaVersion;
        uint8_t *configuration = hwpConfig.Configuration;

        if (dpaVer < 0x0303)
        {
          for (int i = 0; i < CONFIGURATION_LEN; i++)
            configuration[i] = configuration[i] ^ 0x34;
        }

        Document::AllocatorType& allocator = response.GetAllocator();

        // Predefined peripherals - bits
        rapidjson::Value embPerBitsJsonArray(kArrayType);
        for (int i = 0; i < 4; i++)
          embPerBitsJsonArray.PushBack(configuration[i], allocator);
        Pointer("/data/rsp/embPers/values").Set(response, embPerBitsJsonArray);

        // Embedded peripherals bits - parsed
        // Byte 0x01
        uint8_t byte01 = configuration[0x00];

        bool coordPresent = ((byte01 & 0b1) == 0b1);
        Pointer("/data/rsp/embPers/coordinator").Set(response, coordPresent);

        bool nodePresent = ((byte01 & 0b10) == 0b10);
        Pointer("/data/rsp/embPers/node").Set(response, nodePresent);

        bool osPresent = ((byte01 & 0b100) == 0b100);
        Pointer("/data/rsp/embPers/os").Set(response, osPresent);

        bool eepromPresent = ((byte01 & 0b1000) == 0b1000);
        Pointer("/data/rsp/embPers/eeprom").Set(response, eepromPresent);

        bool eeepromPresent = ((byte01 & 0b10000) == 0b10000);
        Pointer("/data/rsp/embPers/eeeprom").Set(response, eeepromPresent);

        bool ramPresent = ((byte01 & 0b100000) == 0b100000);
        Pointer("/data/rsp/embPers/ram").Set(response, ramPresent);

        bool ledrPresent = ((byte01 & 0b1000000) == 0b1000000);
        Pointer("/data/rsp/embPers/ledr").Set(response, ledrPresent);

        bool ledgPresent = ((byte01 & 0b10000000) == 0b10000000);
        Pointer("/data/rsp/embPers/ledg").Set(response, ledgPresent);

        // Byte 0x02
        uint8_t byte02 = configuration[0x01];

        if (dpaVer < 0x0415)
        {
          bool spiPresent = ((byte02 & 0b1) == 0b1);
          Pointer("/data/rsp/embPers/spi").Set(response, spiPresent);
        }

        bool ioPresent = ((byte02 & 0b10) == 0b10);
        Pointer("/data/rsp/embPers/io").Set(response, ioPresent);

        bool thermometerPresent = ((byte02 & 0b100) == 0b100);
        Pointer("/data/rsp/embPers/thermometer").Set(response, thermometerPresent);

        if (dpaVer < 0x0415)
        {
          bool pwmPresent = ((byte02 & 0b1000) == 0b1000);
          Pointer("/data/rsp/embPers/pwm").Set(response, pwmPresent);
        }

        bool uartPresent = ((byte02 & 0b10000) == 0b10000);
        Pointer("/data/rsp/embPers/uart").Set(response, uartPresent);

        if (dpaVer < 0x0400)
        {
          bool frcPresent = ((byte02 & 0b100000) == 0b100000);
          Pointer("/data/rsp/embPers/frc").Set(response, frcPresent);
        }

        // Byte 0x05
        uint8_t byte05 = configuration[0x04];

        bool customDpaHandler = ((byte05 & 0b00000001) == 0b00000001);
        Pointer("/data/rsp/customDpaHandler").Set(response, customDpaHandler);

        // For DPA v4.00 downwards
        if (dpaVer < 0x0400) {
          bool nodeDpaInterface = ((byte05 & 0b00000010) == 0b00000010);
          Pointer("/data/rsp/nodeDpaInterface").Set(response, nodeDpaInterface);
        }

        // For DPA v4.10 upwards
        if (dpaVer >= 0x0410)
        {
          bool dpaPeerToPeer = ((byte05 & 0b00000010) == 0b00000010);
          Pointer("/data/rsp/dpaPeerToPeer").Set(response, dpaPeerToPeer);
        }

        bool dpaAutoexec = ((byte05 & 0b00000100) == 0b00000100);
        Pointer("/data/rsp/dpaAutoexec").Set(response, dpaAutoexec);

        bool routingOff = ((byte05 & 0b00001000) == 0b00001000);
        Pointer("/data/rsp/routingOff").Set(response, routingOff);

        bool ioSetup = ((byte05 & 0b00010000) == 0b00010000);
        Pointer("/data/rsp/ioSetup").Set(response, ioSetup);

        bool peerToPeer = ((byte05 & 0b00100000) == 0b00100000);
        Pointer("/data/rsp/peerToPeer").Set(response, peerToPeer);

        // For DPA v3.03 onwards
        if (dpaVer >= 0x0303)
        {
          bool neverSleep = ((byte05 & 0b01000000) == 0b01000000);
          Pointer("/data/rsp/neverSleep").Set(response, neverSleep);
        }

        // For DPA v4.00 onwards
        if (dpaVer >= 0x0400)
        {
          bool stdAndLpNetwork = ((byte05 & 0b10000000) == 0b10000000);
          Pointer("/data/rsp/stdAndLpNetwork").Set(response, stdAndLpNetwork);
        }

        // Bytes fields
        Pointer("/data/rsp/rfChannelA").Set(response, configuration[0x10]);
        Pointer("/data/rsp/rfChannelB").Set(response, configuration[0x11]);

        // Up to DPA < 4.00
        if (dpaVer < 0x0400)
        {
          Pointer("/data/rsp/rfSubChannelA").Set(response, configuration[0x05]);
          Pointer("/data/rsp/rfSubChannelB").Set(response, configuration[0x06]);
        }

        Pointer("/data/rsp/txPower").Set(response, configuration[0x07]);
        Pointer("/data/rsp/rxFilter").Set(response, configuration[0x08]);
        Pointer("/data/rsp/lpRxTimeout").Set(response, configuration[0x09]);
        Pointer("/data/rsp/rfAltDsmChannel").Set(response, configuration[0x0B]);

        // BaudRate
        if (configuration[0x0A] <= sizeof(BaudRates))
          Pointer("/data/rsp/uartBaudrate").Set(response, BaudRates[configuration[0x0A]]);
        else
        {
          TRC_WARNING("Unknown baud rate constant: " << PAR(configuration[0x0A]));
          Pointer("/data/rsp/uartBaudrate").Set(response, 0);
        }

        // For DPA >= v4.15
        if (dpaVer >= 0x0415)
        {
          // LocalFrcReception at [N] only
          if (m_readTrConfParams.deviceAddress != COORDINATOR_ADDRESS)
          {
            bool localFrcReception = ((configuration[0x0c] & 0b00000001) == 0b00000001);
            Pointer("/data/rsp/localFrcReception").Set(response, localFrcReception);
          }
        }

        // RFPGM byte
        uint8_t rfpgm = hwpConfig.RFPGM;

        bool rfPgmDualChannel = ((rfpgm & 0b00000011) == 0b00000011);
        Pointer("/data/rsp/rfPgmDualChannel").Set(response, rfPgmDualChannel);

        bool rfPgmLpMode = ((rfpgm & 0b00000100) == 0b00000100);
        Pointer("/data/rsp/rfPgmLpMode").Set(response, rfPgmLpMode);

        bool rfPgmIncorrectUpload = ((rfpgm & 0b00001000) == 0b00001000);
        Pointer("/data/rsp/rfPgmIncorrectUpload").Set(response, rfPgmIncorrectUpload);

        bool enableAfterReset = ((rfpgm & 0b00010000) == 0b00010000);
        Pointer("/data/rsp/rfPgmEnableAfterReset").Set(response, enableAfterReset);

        bool rfPgmTerminateAfter1Min = ((rfpgm & 0b01000000) == 0b01000000);
        Pointer("/data/rsp/rfPgmTerminateAfter1Min").Set(response, rfPgmTerminateAfter1Min);

        bool rfPgmTerminateMcuPin = ((rfpgm & 0b10000000) == 0b10000000);
        Pointer("/data/rsp/rfPgmTerminateMcuPin").Set(response, rfPgmTerminateMcuPin);


        // RF band - undocumented byte
        std::string rfBand = "";
        switch (hwpConfig.Undocumented[0] & 0x03)
        {
        case 0b00:
          rfBand = "868";
          break;
        case 0b01:
          rfBand = "916";
          break;
        case 0b10:
          rfBand = "433";
          break;
        default:
          TRC_WARNING("Unknown baud rate constant: " << PAR((hwpConfig.Undocumented[0] & 0x03)));
        }
        Pointer("/data/rsp/rfBand").Set(response, rfBand);

        // Undocumented breakdown for DPA 4.17+
        if (dpaVer >= 0x0417) {
          bool thermometerPresent = hwpConfig.Undocumented[0] & 0x10;
          Pointer("/data/rsp/thermometerSensorPresent").Set(response, thermometerPresent);

          bool eepromPresent = hwpConfig.Undocumented[0] & 0x20;
          Pointer("/data/rsp/serialEepromPresent").Set(response, eepromPresent);

          bool transcieverIL = hwpConfig.Undocumented[0] & 0x40;
          Pointer("/data/rsp/transcieverILType").Set(response, transcieverIL);
        }
      }

      // Set raw fields, if verbose mode is active
      if (m_comReadTrConf->getVerbose())
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = response.GetAllocator();
        while (readTrConfigResult.isNextTransactionResult())
        {
          std::unique_ptr<IDpaTransactionResult2> transResult = readTrConfigResult.consumeNextTransactionResult();
          rapidjson::Value rawObject(kObjectType);
          rawObject.AddMember(
            "request",
            HexStringConversion::encodeBinary(transResult->getRequest().DpaPacket().Buffer, transResult->getRequest().GetLength()),
            allocator
          );
          rawObject.AddMember(
            "requestTs",
            TimeConversion::encodeTimestamp(transResult->getRequestTs()),
            allocator
          );
          rawObject.AddMember(
            "confirmation",
            HexStringConversion::encodeBinary(transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength()),
            allocator
          );
          rawObject.AddMember(
            "confirmationTs",
            TimeConversion::encodeTimestamp(transResult->getConfirmationTs()),
            allocator
          );
          rawObject.AddMember(
            "response",
            HexStringConversion::encodeBinary(transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength()),
            allocator
          );
          rawObject.AddMember(
            "responseTs",
            TimeConversion::encodeTimestamp(transResult->getResponseTs()),
            allocator
          );
          // Add object into array
          rawArray.PushBack(rawObject, allocator);
        }
        // Add array into response document
        Pointer("/data/raw").Set(response, rawArray);
      }

      // Set status
      Pointer("/data/status").Set(response, status);
      Pointer("/data/statusStr").Set(response, readTrConfigResult.getStatusStr());

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messaging, std::move(response));
    }

    //-----------------------
    // Read TR config service
    //-----------------------
    void readTrConfig(ReadTrConfigResult& readTrConfigResult)
    {
      TRC_FUNCTION_ENTER("");
      try
      {
        // Enumerate peripheral
        getPerInfo(readTrConfigResult, m_readTrConfParams.deviceAddress);
        readConfig(readTrConfigResult, m_readTrConfParams.deviceAddress, m_readTrConfParams.hwpId);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
        TRC_FUNCTION_LEAVE("");
      }
    }

    //-------------------
    // Handle the request
    //-------------------
    void handleMsg(const MessagingInstance &messaging, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(
        PAR( messaging.to_string() ) <<
        NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) <<
        NAME_PAR(minor, msgType.m_minor) <<
        NAME_PAR(micro, msgType.m_micro)
      );

      // Unsupported type of request
      if (msgType.m_type != m_mTypeName_iqmeshNetworkReadTrConf)
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));

      // Creating representation object
      ComIqmeshNetworkReadTrConf comReadTrConf(doc);
      m_msgType = &msgType;
      m_messaging = &messaging;
      m_comReadTrConf = &comReadTrConf;

      // Parsing and checking service parameters
      try
      {
        m_readTrConfParams = comReadTrConf.getReadTrConfParams();
      }
      catch (const std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Error while parsing service input parameters.");
        createResponse(parsingRequestError, e.what());
        TRC_FUNCTION_LEAVE("");
        return;
      }

      // Try to establish exclusive access
      try
      {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch (const std::exception &e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Exclusive access error.");
        createResponse(exclusiveAccessError, e.what());
        TRC_FUNCTION_LEAVE("");
        return;
      }

      try
      {
        // Read TR config result
        ReadTrConfigResult readTrConfigResult;

        // Read TR configuration
        readTrConfig(readTrConfigResult);

        // Create and send response
        createResponse(readTrConfigResult);
      } catch (const std::exception& e) {
        m_exclusiveAccess.reset();
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }

      // Release exclusive access
      m_exclusiveAccess.reset();
      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "ReadTrConfService instance activate" << std::endl <<
        "************************************"
      );

      (void)props;

      // for the sake of register function parameters
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkReadTrConf
      };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&](const MessagingInstance &messaging, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
        {
          handleMsg(messaging, msgType, std::move(doc));
        });

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "**************************************" << std::endl <<
        "ReadTrConfService instance deactivate" << std::endl <<
        "**************************************"
      );

      // for the sake of unregister function parameters
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_iqmeshNetworkReadTrConf
      };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(supportedMsgTypes);

      TRC_FUNCTION_LEAVE("");
    }

    void modify(const shape::Properties *props)
    {
      (void)props;
    }

    void attachInterface(IIqrfDpaService* iface)
    {
      m_iIqrfDpaService = iface;
    }

    void detachInterface(IIqrfDpaService* iface)
    {
      if (m_iIqrfDpaService == iface) {
        m_iIqrfDpaService = nullptr;
      }
    }

    void attachInterface(IMessagingSplitterService* iface)
    {
      m_iMessagingSplitterService = iface;
    }

    void detachInterface(IMessagingSplitterService* iface)
    {
      if (m_iMessagingSplitterService == iface) {
        m_iMessagingSplitterService = nullptr;
      }
    }
  };

  ReadTrConfService::ReadTrConfService()
  {
    m_imp = shape_new Imp(*this);
  }

  ReadTrConfService::~ReadTrConfService()
  {
    delete m_imp;
  }

  void ReadTrConfService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void ReadTrConfService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void ReadTrConfService::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void ReadTrConfService::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void ReadTrConfService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void ReadTrConfService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void ReadTrConfService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void ReadTrConfService::deactivate()
  {
    m_imp->deactivate();
  }

  void ReadTrConfService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}
