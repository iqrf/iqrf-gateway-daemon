#define IJsonSerializerService_EXPORTS

#include "JsonSerializer.h"
#include "DpaRaw.h"
#include "DpaMessage.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "JsonUtils.h"
#include <vector>
#include <utility>
#include <algorithm>
#include <stdexcept>

#include "Trace.h"

#include "iqrf__JsonSerializer.hxx"

TRC_INIT_MODULE(iqrf::JsonSerializer);

using namespace rapidjson;

namespace iqrf {
  /// \class PrfCommonJson
  /// \brief Implements common features of JsonDpaMessage
  /// \details
  /// Common functions as parsing and encoding common items of JSON coded DPA messages
  class PrfCommonJson
  {
  protected:

    PrfCommonJson();
    PrfCommonJson(const PrfCommonJson& o);

    /// \brief Parse common items
    /// \param [in] val JSON structure to be parsed
    /// \param [out] dpaTask reference to be set according parsed data
    /// \details
    /// Gets JSON encoded DPA request, parse it and set DpaTask object accordingly
    void parseRequestJson(const rapidjson::Value& val, DpaTask& dpaTask);

    /// \brief Encode begining members of JSON
    /// \param [in] dpaTask reference to be encoded
    /// \details
    /// Gets DPA request common members to be stored at the very beginning of JSON message
    void addResponseJsonPrio1Params(const DpaTask& dpaTask);

    /// \brief Encode middle members of JSON
    /// \param [in] dpaTask reference to be encoded
    /// \details
    /// Gets DPA request common members to be stored in the middle of JSON message
    void addResponseJsonPrio2Params(const DpaTask& dpaTask);

    /// \brief Encode final members of JSON and return it
    /// \param [in] dpaTask reference to be encoded
    /// \return complete JSON encoded message
    /// \details
    /// Gets DPA request common parameters to be stored at the end of JSON message and return complete JSON
    std::string encodeResponseJsonFinal(const DpaTask& dpaTask);

  public:

    /// \brief Parse binary data encoded hexa
    /// \param [out] to buffer for result binary data
    /// \param [in] from hexadecimal string
    /// \param [in] maxlen maximal length of binary data
    /// \return length of result
    /// \details
    /// Gets hexadecimal string in form e.g: "00 a5 b1" (space separation) or "00.a5.b1" (dot separation) and parses to binary data
    int parseBinary(uint8_t* to, const std::string& from, int maxlen);

    /// \brief Parse templated ordinary type T encoded hexa
    /// \param [out] to buffer for result binary data
    /// \param [in] from hexadecimal string
    /// \return length of result
    /// \details
    /// Gets hexadecimal string in form e.g: "00a5b1" and inerpret it as templated ordinary type
    template<typename T>
    void parseHexaNum(T& to, const std::string& from)
    {
      int val = 0;
      std::istringstream istr(from);
      if (istr >> std::hex >> val) {
        to = (T)val;
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format: " << PAR(from));
      }
    }

    /// \brief Encode uint_8 to hexa string
    /// \param [out] to encoded string
    /// \param [in] from value to be encoded
    void encodeHexaNum(std::string& to, uint8_t from);

    /// \brief Encode uint_16 to hexa string
    /// \param [out] to encoded string
    /// \param [in] from value to be encoded
    void encodeHexaNum(std::string& to, uint16_t from);

    /// \brief Encode binary data to hexa string
    /// \param [out] to result string
    /// \param [in] from data to be encoded
    /// \param [in] len length of dat to be encoded
    /// \details
    /// Encode binary data to hexadecimal string in form e.g: "00 a5 b1" (space separation) or "00.a5.b1" (dot separation)
    /// Used separation is controlled by member m_dotNotation and it is hardcoded as dot separation it this version
    void encodeBinary(std::string& to, const uint8_t* from, int len);

    /// \brief Encode timestamp
    /// \param [out] to result string
    /// \param [in] from timestamp to be encoded
    void encodeTimestamp(std::string& to, std::chrono::time_point<std::chrono::system_clock> from);

    /// various flags to store presence of members of DPA request to be used in DPA response
    bool m_has_ctype = false;
    bool m_has_type = false;
    bool m_has_nadr = false;
    bool m_has_hwpid = false;
    bool m_has_timeout = false;
    bool m_has_msgid = false;
    bool m_has_request = false;
    bool m_has_request_ts = false;
    bool m_has_response = false;
    bool m_has_response_ts = false;
    bool m_has_confirmation = false;
    bool m_has_confirmation_ts = false;
    bool m_has_cmd = false;
    bool m_has_rcode = false;
    bool m_has_rdata = false;
    bool m_has_dpaval = false;

    /// various flags to store members of DPA request to be used in DPA response
    std::string m_ctype;
    std::string m_type;
    std::string m_nadr = "0";
    std::string m_hwpid = "0xffff";
    int m_timeoutJ = 0;
    std::string m_msgid;
    std::string m_requestJ;
    std::string m_request_ts;
    std::string m_responseJ;
    std::string m_response_ts;
    std::string m_confirmationJ;
    std::string m_confirmation_ts;
    std::string m_cmdJ;
    std::string m_statusJ;
    std::string m_rcodeJ;
    std::string m_rdataJ;
    std::string m_dpavalJ;

    rapidjson::Document m_doc;

    bool m_dotNotation = true;
  };

  /// \class PrfRawJson
  /// \brief Parse/encode JSON message holding DpaRaw
  /// \details
  /// Class to be passed to parser as creator of DpaRaw object from incoming JSON.
  /// See https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#raw for details
  class PrfRawJson : public DpaRaw, public PrfCommonJson
  {
  public:
    /// \brief parametric constructor
    /// \param [in] val JSON to be parsed
    explicit PrfRawJson(const rapidjson::Value& val);

    /// \brief parametric constructor
    /// \param [in] dpaMessage DPA async. message
    /// \details
    /// This is used to create PrfRaw from asynchronous message
    explicit PrfRawJson(const DpaMessage& dpaMessage);
    virtual ~PrfRawJson() {}

    /// \brief DpaTask overriden method
    /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
    /// \return encoded message
    std::string encodeResponse(const std::string& errStr) override;

    /// \brief DpaTask overriden method
    /// \param [in] errStr result of DpaTask handling in IQRF mesh - asynchronous
    /// \return encoded message
    std::string encodeAsyncRequest(const std::string& errStr);
  private:
  };

  /// \class PrfRawJson
  /// \brief Parse/encode JSON message holding DpaRaw
  /// \details
  /// Class to be passed to parser as creator of DpaRaw object from incoming JSON. It parses/encodes DPA header.
  /// See https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#raw-hdp for details
  class PrfRawHdpJson : public DpaRaw, public PrfCommonJson
  {
  public:
    /// name to be registered in parser
    /// expected in JSON as: "type": "raw-hdp"
    static const std::string PRF_NAME;

    /// \brief parametric constructor
    /// \param [in] val JSON to be parsed
    explicit PrfRawHdpJson(const rapidjson::Value& val);
    virtual ~PrfRawHdpJson() {}

    /// \brief DpaTask overriden method
    /// \param [in] errStr result of DpaTask handling in IQRF mesh to be stored in message
    /// \return encoded message
    std::string encodeResponse(const std::string& errStr) override;
  private:
    /// parsed DPA header items
    std::string m_pnum;
    std::string m_pcmd;
    std::string m_data;

  };

#define CTYPE_STR "ctype"
#define TYPE_STR "type"
#define NADR_STR "nadr"
#define HWPID_STR "hwpid"
#define TIMEOUT_STR "timeout"
#define MSGID_STR "msgid"
#define REQUEST_STR "request"
#define REQUEST_TS_STR "request_ts"
#define RESPONSE_STR "response"
#define RESPONSE_TS_STR "response_ts"
#define CONFIRMATION_STR "confirmation"
#define CONFIRMATION_TS_STR "confirmation_ts"
#define CMD_STR "cmd"
#define RCODE_STR "rcode"
#define RESD_STR "rdata"
#define DPAVAL_STR "dpaval"
#define STATUS_STR "status"

  //////////////////////////////////////////
  PrfCommonJson::PrfCommonJson()
  {
    m_doc.SetObject();
  }

  PrfCommonJson::PrfCommonJson(const PrfCommonJson& o)
  {
    m_has_ctype = o.m_has_ctype;
    m_has_type = o.m_has_type;
    m_has_nadr = o.m_has_nadr;
    m_has_hwpid = o.m_has_hwpid;
    m_has_timeout = o.m_has_timeout;
    m_has_msgid = o.m_has_msgid;
    m_has_request = o.m_has_request;
    m_has_request_ts = o.m_has_request_ts;
    m_has_response = o.m_has_response;
    m_has_response_ts = o.m_has_response_ts;
    m_has_confirmation = o.m_has_confirmation;
    m_has_confirmation_ts = o.m_has_confirmation_ts;
    m_has_cmd = o.m_has_cmd;
    m_has_rcode = o.m_has_rcode;
    m_has_dpaval = o.m_has_dpaval;

    m_ctype = o.m_ctype;
    m_type = o.m_type;
    m_nadr = o.m_nadr;
    m_hwpid = o.m_hwpid;
    m_timeoutJ = o.m_timeoutJ;
    m_msgid = o.m_msgid;
    m_requestJ = o.m_requestJ;
    m_request_ts = o.m_request_ts;
    m_responseJ = o.m_responseJ;
    m_response_ts = o.m_response_ts;
    m_confirmationJ = o.m_confirmationJ;
    m_confirmation_ts = o.m_confirmation_ts;
    m_cmdJ = o.m_cmdJ;
    m_statusJ = o.m_statusJ;
    m_rcodeJ = o.m_rcodeJ;
    m_dpavalJ = o.m_dpavalJ;

    m_doc.SetObject();
  }

  int PrfCommonJson::parseBinary(uint8_t* to, const std::string& from, int maxlen)
  {
    int retval = 0;
    if (!from.empty()) {
      std::string buf = from;
      if (std::string::npos != buf.find_first_of('.')) {
        std::replace(buf.begin(), buf.end(), '.', ' ');
        m_dotNotation = true;
      }
      std::istringstream istr(buf);

      int val;
      while (retval < maxlen) {
        if (!(istr >> std::hex >> val)) {
          if (istr.eof()) break;
          THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format: " << PAR(from));
        }
        to[retval++] = (uint8_t)val;
      }
    }
    return retval;
  }

  void PrfCommonJson::encodeHexaNum(std::string& to, uint8_t from)
  {
    std::ostringstream os;
    os.fill('0'); os.width(2);
    os << std::hex << (int)from;
    to = os.str();
  }

  void PrfCommonJson::encodeHexaNum(std::string& to, uint16_t from)
  {
    std::ostringstream os;
    os.fill('0'); os.width(4);
    os << std::hex << (int)from;
    to = os.str();
  }

  void PrfCommonJson::encodeBinary(std::string& to, const uint8_t* from, int len)
  {
    bool dot = std::string::npos != to.find_first_of('.');
    to.clear();
    if (len > 0) {
      std::ostringstream ostr;
      ostr << shape::TracerMemHex(from, len, '.');

      if (m_dotNotation || dot) {
        to = ostr.str();
        std::replace(to.begin(), to.end(), ' ', '.');
        if (to[to.size() - 1] == '.')
          to.pop_back();
      }
      else {
        to = ostr.str();
        if (to[to.size() - 1] == ' ')
          to.pop_back();
      }
    }
  }

  void PrfCommonJson::encodeTimestamp(std::string& to, std::chrono::time_point<std::chrono::system_clock> from)
  {
    using namespace std::chrono;

    to.clear();
    if (from.time_since_epoch() != system_clock::duration()) {
      auto fromUs = std::chrono::duration_cast<std::chrono::microseconds>(from.time_since_epoch()).count() % 1000000;
      auto time = std::chrono::system_clock::to_time_t(from);
      //auto tm = *std::gmtime(&time);
      auto tm = *std::localtime(&time);

      char buf[80];
      strftime(buf, sizeof(buf), "%FT%T", &tm);

      std::ostringstream os;
      os.fill('0'); os.width(6);
      //os << std::put_time(&tm, "%F %T.") <<  fromUs; // << std::put_time(&tm, " %Z\n");
      os << buf << "." << fromUs;

      to = os.str();
    }
  }

  void PrfCommonJson::parseRequestJson(const rapidjson::Value& val, DpaTask& dpaTask)
  {
    jutils::assertIsObject("", val);

    m_has_ctype = jutils::getMemberIfExistsAs<std::string>(CTYPE_STR, val, m_ctype);
    m_has_type = jutils::getMemberIfExistsAs<std::string>(TYPE_STR, val, m_type);
    m_has_nadr = jutils::getMemberIfExistsAs<std::string>(NADR_STR, val, m_nadr);
    m_has_hwpid = jutils::getMemberIfExistsAs<std::string>(HWPID_STR, val, m_hwpid);
    m_has_timeout = jutils::getMemberIfExistsAs<int>(TIMEOUT_STR, val, m_timeoutJ);
    m_has_msgid = jutils::getMemberIfExistsAs<std::string>(MSGID_STR, val, m_msgid);
    m_has_request = jutils::getMemberIfExistsAs<std::string>(REQUEST_STR, val, m_requestJ);
    m_has_request_ts = jutils::getMemberIfExistsAs<std::string>(REQUEST_TS_STR, val, m_request_ts);
    m_has_response = jutils::getMemberIfExistsAs<std::string>(RESPONSE_STR, val, m_responseJ);
    m_has_response_ts = jutils::getMemberIfExistsAs<std::string>(RESPONSE_TS_STR, val, m_response_ts);
    m_has_confirmation = jutils::getMemberIfExistsAs<std::string>(CONFIRMATION_STR, val, m_confirmationJ);
    m_has_confirmation_ts = jutils::getMemberIfExistsAs<std::string>(CONFIRMATION_TS_STR, val, m_confirmation_ts);
    m_has_cmd = jutils::getMemberIfExistsAs<std::string>(CMD_STR, val, m_cmdJ);
    m_has_rcode = jutils::getMemberIfExistsAs<std::string>(RCODE_STR, val, m_rcodeJ);
    m_has_dpaval = jutils::getMemberIfExistsAs<std::string>(DPAVAL_STR, val, m_dpavalJ);

    if (m_has_nadr) {
      uint16_t nadr;
      parseHexaNum(nadr, m_nadr);
      dpaTask.setAddress(nadr);
    }
    if (m_has_hwpid) {
      uint16_t hwpid;
      parseHexaNum(hwpid, m_hwpid);
      dpaTask.setHwpid(hwpid);
    }
    if (m_has_cmd) {
      dpaTask.parseCommand(m_cmdJ);
    }
    if (m_has_timeout && m_timeoutJ >= 0) {
      dpaTask.setTimeout(m_timeoutJ);
    }
  }

  void PrfCommonJson::addResponseJsonPrio1Params(const DpaTask& dpaTask)
  {
    Document::AllocatorType& alloc = m_doc.GetAllocator();
    rapidjson::Value v;

    bool responded = true;
    if (0 >= dpaTask.getResponse().GetLength())
      responded = false;

    if (m_has_ctype) {
      v.SetString(m_ctype.c_str(), alloc);
      m_doc.AddMember(CTYPE_STR, v, alloc);
    }
    if (m_has_type) {
      v.SetString(m_type.c_str(), alloc);
      m_doc.AddMember(TYPE_STR, v, alloc);
    }
    if (m_has_msgid) {
      v.SetString(m_msgid.c_str(), alloc);
      m_doc.AddMember(MSGID_STR, v, alloc);
    }
    if (m_has_timeout) {
      v = m_timeoutJ;
      m_doc.AddMember(TIMEOUT_STR, v, alloc);
    }
    if (m_has_nadr) {
      if (!responded)
        m_nadr.clear();
      v.SetString(m_nadr.c_str(), alloc);
      m_doc.AddMember(NADR_STR, v, alloc);
    }
  }

  void PrfCommonJson::addResponseJsonPrio2Params(const DpaTask& dpaTask)
  {
    Document::AllocatorType& alloc = m_doc.GetAllocator();
    rapidjson::Value v;

    bool responded = true;
    if (0 >= dpaTask.getResponse().GetLength())
      responded = false;

    if (m_has_cmd) {
      if (!responded)
        m_cmdJ.clear();
      v.SetString(m_cmdJ.c_str(), alloc);
      m_doc.AddMember(CMD_STR, v, alloc);
    }
    if (m_has_hwpid) {
      if (!responded)
        m_hwpid.clear();
      v.SetString(m_hwpid.c_str(), alloc);
      m_doc.AddMember(HWPID_STR, v, alloc);
    }
  }

  std::string PrfCommonJson::encodeResponseJsonFinal(const DpaTask& dpaTask)
  {
    Document::AllocatorType& alloc = m_doc.GetAllocator();
    rapidjson::Value v;

    bool responded = true;
    if (0 >= dpaTask.getResponse().GetLength())
      responded = false;

    if (m_has_rcode) {
      if (responded)
        encodeHexaNum(m_rcodeJ, dpaTask.getResponse().DpaPacket().DpaResponsePacket_t.ResponseCode);
      else
        m_rcodeJ.clear();
      v.SetString(m_rcodeJ.c_str(), alloc);
      m_doc.AddMember(RCODE_STR, v, alloc);
    }

    if (m_has_dpaval) {
      if (responded)
        encodeHexaNum(m_dpavalJ, dpaTask.getResponse().DpaPacket().DpaResponsePacket_t.DpaValue);
      else
        m_dpavalJ.clear();
      v.SetString(m_dpavalJ.c_str(), alloc);
      m_doc.AddMember(DPAVAL_STR, v, alloc);
    }

    if (m_has_rdata) {
      if (responded) {
        int datalen = dpaTask.getResponse().GetLength() - sizeof(TDpaIFaceHeader) - 2; //DpaValue ResponseCode
        if (datalen > 0) {
          encodeBinary(m_rdataJ, dpaTask.getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData, datalen);
        }
      }
      else
        m_rdataJ.clear();
      v.SetString(m_rdataJ.c_str(), alloc);
      m_doc.AddMember(RESD_STR, v, alloc);
    }

    if (m_has_request) {
      encodeBinary(m_requestJ, dpaTask.getRequest().DpaPacket().Buffer, dpaTask.getRequest().GetLength());
      v.SetString(m_requestJ.c_str(), alloc);
      m_doc.AddMember(REQUEST_STR, v, alloc);
    }
    if (m_has_request_ts) {
      encodeTimestamp(m_request_ts, dpaTask.getRequestTs());
      v.SetString(m_request_ts.c_str(), alloc);
      m_doc.AddMember(REQUEST_TS_STR, v, alloc);
    }
    if (m_has_confirmation) {
      encodeBinary(m_confirmationJ, dpaTask.getConfirmation().DpaPacket().Buffer, dpaTask.getConfirmation().GetLength());
      v.SetString(m_confirmationJ.c_str(), alloc);
      m_doc.AddMember(CONFIRMATION_STR, v, alloc);
    }
    if (m_has_confirmation_ts) {
      encodeTimestamp(m_confirmation_ts, dpaTask.getConfirmationTs());
      v.SetString(m_confirmation_ts.c_str(), alloc);
      m_doc.AddMember(CONFIRMATION_TS_STR, v, alloc);
    }
    if (m_has_response) {
      encodeBinary(m_responseJ, dpaTask.getResponse().DpaPacket().Buffer, dpaTask.getResponse().GetLength());
      v.SetString(m_responseJ.c_str(), alloc);
      m_doc.AddMember(RESPONSE_STR, v, alloc);
    }
    if (m_has_response_ts) {
      encodeTimestamp(m_response_ts, dpaTask.getResponseTs());
      v.SetString(m_response_ts.c_str(), alloc);
      m_doc.AddMember(RESPONSE_TS_STR, v, alloc);
    }

    v.SetString(m_statusJ.c_str(), alloc);
    m_doc.AddMember(STATUS_STR, v, alloc);

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    m_doc.Accept(writer);
    return buffer.GetString();
  }

  /////////////////////////////////////////
  //-------------------------------
  PrfRawJson::PrfRawJson(const rapidjson::Value& val)
  {
    parseRequestJson(val, *this);

    if (!m_has_request) {
      THROW_EXC_TRC_WAR(std::logic_error, "Missing member: " << REQUEST_STR);
    }

    int len = parseBinary(m_request.DpaPacket().Buffer, m_requestJ, MAX_DPA_BUFFER);
    m_request.SetLength(len);
  }

  std::string PrfRawJson::encodeResponse(const std::string& errStr)
  {
    if (m_dotNotation) {
      m_responseJ = ".";
    }
    m_has_response = true; //mandatory here
    m_statusJ = errStr;

    addResponseJsonPrio1Params(*this);
    addResponseJsonPrio2Params(*this);
    return encodeResponseJsonFinal(*this);
  }

  std::string PrfRawJson::encodeAsyncRequest(const std::string& errStr)
  {
    if (m_dotNotation) {
      m_responseJ = ".";
    }
    m_has_response = false; //unwanted here
    m_statusJ = errStr;

    addResponseJsonPrio1Params(*this);
    addResponseJsonPrio2Params(*this);
    return encodeResponseJsonFinal(*this);
  }

  //just for Async
  PrfRawJson::PrfRawJson(const DpaMessage& dpaMessage)
  {
    m_ctype = CAT_DPA_STR;
    m_type = DpaRaw::PRF_NAME;
    switch (dpaMessage.MessageDirection()) {
    case DpaMessage::MessageType::kRequest:
      setRequest(dpaMessage);
      m_has_request = true;
      m_has_response = true;
      m_has_request_ts = true;
      timestampRequest();
      break;
    case DpaMessage::MessageType::kResponse:
      parseResponse(dpaMessage);
      m_has_request = true;
      m_has_response = true;
      m_has_response_ts = true;
      timestampResponse();
      break;
    default:;
    }
    m_has_ctype = true;
    m_has_type = true;
  }

  //-------------------------------
  const std::string PrfRawHdpJson::PRF_NAME("raw-hdp");

#define PNUM_STR "pnum"
#define PCMD_STR "pcmd"
#define REQD_STR "rdata"

  PrfRawHdpJson::PrfRawHdpJson(const rapidjson::Value& val)
  {
    parseRequestJson(val, *this);

    //mandatory here
    m_pnum = jutils::getMemberAs<std::string>(PNUM_STR, val);
    m_pcmd = jutils::getMemberAs<std::string>(PCMD_STR, val);
    m_hwpid = jutils::getMemberAs<std::string>(HWPID_STR, val);

    m_data = jutils::getPossibleMemberAs<std::string>(REQD_STR, val, m_data);

    {
      uint8_t pnum;
      parseHexaNum(pnum, m_pnum);
      m_request.DpaPacket().DpaRequestPacket_t.PNUM = pnum;
    }

    {
      uint8_t pcmd;
      parseHexaNum(pcmd, m_pcmd);
      m_request.DpaPacket().DpaRequestPacket_t.PCMD = pcmd;
    }

    {
      uint16_t hwpid;
      parseHexaNum(hwpid, m_hwpid);
      m_request.DpaPacket().DpaRequestPacket_t.HWPID = hwpid;
    }

    int len = parseBinary(m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData, m_data, DPA_MAX_DATA_LENGTH);
    m_request.SetLength(sizeof(TDpaIFaceHeader) + len);

  }

  std::string PrfRawHdpJson::encodeResponse(const std::string& errStr)
  {
    Document::AllocatorType& alloc = m_doc.GetAllocator();
    rapidjson::Value v;

    addResponseJsonPrio1Params(*this);

    bool responded = true;
    if (0 >= getResponse().GetLength())
      responded = false;

    uint8_t pnum = getResponse().DpaPacket().DpaResponsePacket_t.PNUM;
    uint8_t pcmd = getResponse().DpaPacket().DpaResponsePacket_t.PCMD;
    uint16_t hwpid = getResponse().DpaPacket().DpaResponsePacket_t.HWPID;
    m_has_hwpid = true;

    if (responded) {
      encodeHexaNum(m_pnum, pnum);
      encodeHexaNum(m_pcmd, pcmd);
      encodeHexaNum(m_hwpid, hwpid);
    }
    else {
      m_pnum.clear();
      m_pcmd.clear();
      m_hwpid.clear();
    }

    v.SetString(m_pnum.c_str(), alloc);
    m_doc.AddMember(PNUM_STR, v, alloc);

    v.SetString(m_pcmd.c_str(), alloc);
    m_doc.AddMember(PCMD_STR, v, alloc);

    //mandatory here
    m_has_rcode = true;
    m_has_rdata = true;
    m_has_dpaval = true;

    m_statusJ = errStr;

    addResponseJsonPrio2Params(*this);
    return encodeResponseJsonFinal(*this);
  }

  ///////////////////////////////////////////////
  JsonSerializer::JsonSerializer()
  {
    TRC_FUNCTION_ENTER("");

    registerClass<PrfRawJson>(DpaRaw::PRF_NAME);
    registerClass<PrfRawHdpJson>(PrfRawHdpJson::PRF_NAME);

    TRC_FUNCTION_LEAVE("")
  }

  JsonSerializer::~JsonSerializer()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  std::string JsonSerializer::parseCategory(const std::string& request)
  {
    std::string ctype;
    try {
      Document doc;
      jutils::parseString(request, doc);

      jutils::assertIsObject("", doc);
      ctype = jutils::getMemberAs<std::string>("ctype", doc);
    }
    catch (std::exception &e) {
      m_lastError = e.what();
    }
    return ctype;
  }

  std::unique_ptr<DpaTask> JsonSerializer::parseRequest(const std::string& request)
  {
    std::unique_ptr<DpaTask> obj;
    try {
      Document doc;
      jutils::parseString(request, doc);

      jutils::assertIsObject("", doc);
      std::string perif = jutils::getMemberAs<std::string>("type", doc);

      obj = createObject(perif, doc);
    }
    catch (std::exception &e) {
      m_lastError = e.what();
    }
    return std::move(obj);
  }

  std::string JsonSerializer::parseConfig(const std::string& request)
  {
    std::string cmd;
    try {
      Document doc;
      jutils::parseString(request, doc);

      jutils::assertIsObject("", doc);

      std::string type = jutils::getMemberAs<std::string>("type", doc);

      if (type == "mode") {
        cmd = jutils::getMemberAs<std::string>("cmd", doc);
      }
    }
    catch (std::exception &e) {
      m_lastError = e.what();
    }
    return cmd;
  }

  std::string JsonSerializer::encodeConfig(const std::string& request, const std::string& response)
  {
    std::string res;
    try {
      Document doc;

      jutils::parseString(request, doc);
      jutils::assertIsObject("", doc);

      Document::AllocatorType& alloc = doc.GetAllocator();
      rapidjson::Value v;
      v.SetString(response.c_str(), alloc);
      doc.AddMember("status", v, alloc);

      StringBuffer buffer;
      PrettyWriter<StringBuffer> writer(buffer);
      doc.Accept(writer);
      res = buffer.GetString();
    }
    catch (std::exception &e) {
      m_lastError = e.what();
    }
    return res;
  }

  std::string JsonSerializer::getLastError() const
  {
    return m_lastError;
  }

  std::string JsonSerializer::encodeAsyncAsDpaRaw(const DpaMessage& dpaMessage) const
  {
    PrfRawJson raw(dpaMessage);
    raw.m_dotNotation = true;
    std::string status;
    switch (dpaMessage.MessageDirection()) {
    case DpaMessage::MessageType::kRequest:
      raw.m_has_request = true;
      raw.m_has_response = false;
      status = "ASYNC_REQUEST";
      return raw.encodeAsyncRequest(status);
    case DpaMessage::MessageType::kResponse:
      raw.m_has_request = false;
      raw.m_has_response = true;
      status = "ASYNC_RESPONSE";
      return raw.encodeResponse(status);
    default:
      status = "ASYNC_MESSAGE";
    }
    return raw.encodeResponse(status);
  }
  
  ///////////////////////////////////////////////////////////////
  void JsonSerializer::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "JsonSerializer instance activate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void JsonSerializer::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "JsonSerializer instance deactivate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void JsonSerializer::modify(const shape::Properties *props)
  {
  }

  void JsonSerializer::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonSerializer::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
