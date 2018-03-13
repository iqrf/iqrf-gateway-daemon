#define IMessagingSplitterService_EXPORTS

#include "DpaHandler2.h"
#include "JsonDpaApi.h"
#include "JsonUtils.h"
#include "rapidjson/schema.h"
#include "ITemplateService.h"
#include "Trace.h"
#include <algorithm>

#include "iqrf__JsonDpaApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApi);

using namespace rapidjson;
using namespace jutils;

namespace iqrf {
  class ComBase
  {
  public:
    ComBase() {}
    ComBase(rapidjson::Document& doc)
    {
      m_msgId = Pointer("/data/msgId").Get(doc)->GetString();
      m_timeout = Pointer("/data/timeout").GetWithDefault(doc, (int)m_timeout).GetInt();
      m_verbose = Pointer("/data/returnVerbose").GetWithDefault(doc, m_verbose).GetBool();
    }
    
    virtual ~ComBase()
    {

    }

    const std::string& getMsgId() const
    {
      return m_msgId;
    }

    const int32_t getTimeout() const
    {
      return m_timeout;
    }

    const bool getVerbose() const
    {
      return m_verbose;
    }

    const DpaMessage& getDpaRequest() const
    {
      return m_request;
    }

    void createResponse(rapidjson::Document& doc, const IDpaTransactionResult2& res)
    {
      Pointer("/mType").Set(doc, "TODO-mType"); //TODO
      Pointer("/data/msgId").Set(doc, m_msgId);
      if (m_verbose) {
        Pointer("/data/timeout").Set(doc, m_timeout);
      }

      createResponsePayload(doc, res);

      if (m_verbose) {
        Pointer("/data/raw/request").Set(doc, encodeBinary(res.getRequest().DpaPacket().Buffer, res.getRequest().GetLength()));
        Pointer("/data/raw/requestTs").Set(doc, encodeTimestamp(res.getRequestTs()));
        Pointer("/data/raw/confirmation").Set(doc, encodeBinary(res.getConfirmation().DpaPacket().Buffer, res.getConfirmation().GetLength()));
        Pointer("/data/raw/confirmationTs").Set(doc, encodeTimestamp(res.getConfirmationTs()));
        Pointer("/data/raw/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
        Pointer("/data/raw/responseTs").Set(doc, encodeTimestamp(res.getResponseTs()));

        Pointer("/data/insId").Set(doc, "TODO-insId"); //TODO
        Pointer("/data/statusStr").Set(doc, res.getErrorString());
      }

      Pointer("/data/status").Set(doc, res.getErrorCode());
    }

  protected:

    virtual void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) = 0;
    /// \brief Parse binary data encoded hexa
    /// \param [out] to buffer for result binary data
    /// \param [in] from hexadecimal string
    /// \param [in] maxlen maximal length of binary data
    /// \return length of result
    /// \details
    /// Gets hexadecimal string in form e.g: "00 a5 b1" (space separation) or "00.a5.b1" (dot separation) and parses to binary data
    int parseBinary(uint8_t* to, const std::string& from, int maxlen) const
    {
      int retval = 0;
      if (!from.empty()) {
        std::string buf = from;
        std::replace(buf.begin(), buf.end(), '.', ' ');
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

    /// \brief Parse templated ordinary type T encoded hexa
    /// \param [out] to buffer for result binary data
    /// \param [in] from hexadecimal string
    /// \return length of result
    /// \details
    /// Gets hexadecimal string in form e.g: "00a5b1" and inerpret it as templated ordinary type
    template<typename T>
    T parseHexaNum(T& to, const std::string& from)
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
    std::string encodeHexaNum(uint8_t from) const
    {
      std::ostringstream os;
      os.fill('0'); os.width(2);
      os << std::hex << (int)from;
      return os.str();
    }

    /// \brief Encode uint_16 to hexa string
    /// \param [out] to encoded string
    /// \param [in] from value to be encoded
    std::string encodeHexaNum(uint16_t from) const
    {
      std::ostringstream os;
      os.fill('0'); os.width(4);
      os << std::hex << (int)from;
      return os.str();
    }

    /// \brief Encode binary data to hexa string
    /// \param [out] to result string
    /// \param [in] from data to be encoded
    /// \param [in] len length of dat to be encoded
    /// \details
    /// Encode binary data to hexadecimal string in form e.g: "00 a5 b1" (space separation) or "00.a5.b1" (dot separation)
    /// Used separation is controlled by member m_dotNotation and it is hardcoded as dot separation it this version
    std::string encodeBinary(const uint8_t* from, int len) const
    {
      std::string to;
      if (len > 0) {
        std::ostringstream ostr;
        ostr << shape::TracerMemHex(from, len, '.');
        to = ostr.str();
        if (to[to.size() - 1] == '.') {
          to.pop_back();
        }
      }
      return to;
    }

    /// \brief Encode timestamp
    /// \param [out] to result string
    /// \param [in] from timestamp to be encoded
    std::string encodeTimestamp(std::chrono::time_point<std::chrono::system_clock> from) const
    {
      using namespace std::chrono;

      std::string to;
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
      return to;
    }

    DpaMessage m_request;

  private:
    std::string m_msgId;
    int32_t m_timeout = -1;
    bool m_verbose = false;
  };

  class ComRaw : public ComBase
  {
  public:
    ComRaw() = delete;
    ComRaw(rapidjson::Document& doc)
      :ComBase(doc)
    {
      int len = parseBinary(m_request.DpaPacket().Buffer,
        Pointer("/data/req/request").Get(doc)->GetString(),
        DPA_MAX_DATA_LENGTH);
      m_request.SetLength(len);
    }

    virtual ~ComRaw()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
  };

  class ComRawHdp : public ComBase
  {
  public:
    ComRawHdp() = delete;
    ComRawHdp(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parseHexaNum(m_request.DpaPacket().DpaRequestPacket_t.NADR, Pointer("/data/req/nAdr").Get(doc)->GetString());
      parseHexaNum(m_request.DpaPacket().DpaRequestPacket_t.PNUM, Pointer("/data/req/pNum").Get(doc)->GetString());
      parseHexaNum(m_request.DpaPacket().DpaRequestPacket_t.PCMD, Pointer("/data/req/pCmd").Get(doc)->GetString());
      parseHexaNum(m_request.DpaPacket().DpaRequestPacket_t.HWPID, Pointer("/data/req/hwpId").Get(doc)->GetString());
      int len = parseBinary(m_request.DpaPacket().DpaRequestPacket_t.DpaMessage.Request.PData,
        Pointer("/data/req/rData").Get(doc)->GetString(),
        DPA_MAX_DATA_LENGTH);
      m_request.SetLength(sizeof(TDpaIFaceHeader) + len);
    }

    virtual ~ComRawHdp()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      bool r = res.isResponded();
      Pointer("/data/rsp/nAdr").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.NADR) : "");
      Pointer("/data/rsp/pNum").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.PNUM) : "");
      Pointer("/data/rsp/pCmd").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.PCMD) : "");
      Pointer("/data/rsp/hwpId").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.HWPID) : "");
      Pointer("/data/rsp/rCode").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.ResponseCode) : "");
      Pointer("/data/rsp/dpaVal").Set(doc, r ? encodeHexaNum(res.getResponse().DpaPacket().DpaResponsePacket_t.DpaValue) : "");
      Pointer("/data/rsp/rData").Set(doc, r ? encodeBinary(res.getResponse().DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData,
        res.getResponse().GetLength() - sizeof(TDpaIFaceHeader) - 2) : "");
    }
  };

  /////////////////////////////
  class JsonDpaApi::Imp
  {
  private:
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::list<std::string> m_supportedMsgTypes = { "comRaw", "comRawHdp" };
    std::map<std::string, SchemaDocument> m_validatorMap;

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void createValidator(const std::string& msgType, const std::string& fname)
    {
      Document sd;
      parseJsonFile(fname, sd);
      SchemaDocument schema(sd);
      m_validatorMap.insert(std::make_pair(msgType, std::move(schema)));
    }

    void validate(const std::string& msgType, const Document& doc)
    {
      auto found = m_validatorMap.find(msgType);
      if (found != m_validatorMap.end()) {
        SchemaValidator validator(found->second);
        if (!doc.Accept(validator)) {
          // Input JSON is invalid according to the schema
          // Output diagnostic information
          StringBuffer sb;
          validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
          printf("Invalid schema: %s\n", sb.GetString());
          printf("Invalid keyword: %s\n", validator.GetInvalidSchemaKeyword());
          sb.Clear();
          validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
          printf("Invalid document: %s\n", sb.GetString());
          THROW_EXC_TRC_WAR(std::logic_error, "Invalid");
        }
        TRC_DEBUG("OK");
      }
      else {
        //TODO why
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot find validator");
      }
    }

    void handleMsg(const std::string & messagingId, const std::string & msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << PAR(msgType));
      validate(msgType, doc);
      Document respDoc;
      std::unique_ptr<ComBase> com;

      if (msgType == "comRaw") {
        com.reset(shape_new ComRaw(doc));
      }
      else if (msgType == "comRawHdp") {
        com.reset(shape_new ComRawHdp(doc));
      }

      auto trn = m_iIqrfDpaService->executeDpaTransaction(com->getDpaRequest());
      auto res = trn->get();
      com->createResponse(respDoc, *res);

      //update message type - type is the same for request/response
      Pointer("/mType").Set(respDoc, msgType);

      //TODO validate response in debug
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApi instance activate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->registerFilteredMsgHandler(m_supportedMsgTypes,
        [&](const std::string & messagingId, const std::string & msgType, rapidjson::Document doc)
      {
        handleMsg(messagingId, msgType, std::move(doc));
      });

      ////////////////////////////
      createValidator("comRaw", "./configuration/JsonSchemes/async-api-json-schemes/comRaw-request.json");
      createValidator("comRawHdp", "./configuration/JsonSchemes/async-api-json-schemes/comRawHdp-request.json");
      ////////////////////////////

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApi instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_supportedMsgTypes);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
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

  /////////////////////////
  JsonDpaApi::JsonDpaApi()
  {
    m_imp = shape_new Imp();
  }

  JsonDpaApi::~JsonDpaApi()
  {
    delete m_imp;
  }

  void JsonDpaApi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonDpaApi::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonDpaApi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonDpaApi::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApi::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApi::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApi::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonDpaApi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
