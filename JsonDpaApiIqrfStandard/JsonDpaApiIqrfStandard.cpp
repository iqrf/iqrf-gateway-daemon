#define IMessagingSplitterService_EXPORTS

#include "JsonApiMessageNames.h"
#include "ComIqrfStandard.h"
#include "DpaHandler2.h"
#include "JsonDpaApiIqrfStandard.h"
#include "DuktapeStuff.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "ITemplateService.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>

#include "iqrf__JsonDpaApiIqrfStandard.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApiIqrfStandard);

using namespace rapidjson;

namespace iqrf {
  class JsonDpaApiIqrfStandard::Imp
  {
  private:

    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;

    // TODO from cfg
    std::vector<std::string> m_filters =
    {
      "iqrfEmbed",
      "iqrfLight",
      "iqrfSensor",
    };

    DuktapeStuff m_duk;

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    //for debug only
    static std::string JsonToStr(const rapidjson::Value* val)
    {
      rapidjson::Document doc;
      doc.CopyFrom(*val, doc.GetAllocator());
      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      return buffer.GetString();
    }

    // nadr, hwpid not set from drivers
    std::vector<uint8_t> rawHdpRequestToDpaRequest(int nadr, int hwpid, const std::string& rawHdpRequest)
    {
      using namespace rapidjson;

      std::vector<uint8_t> retvect;

      Document doc;
      doc.Parse(rawHdpRequest);

      uint8_t pnum = 0, pcmd = 0;

      if (Value *val = Pointer("/pnum").Get(doc)) {
        parseHexaNum(pnum, val->GetString());
      }
      if (Value *val = Pointer("/pcmd").Get(doc)) {
        parseHexaNum(pcmd, val->GetString());
      }

      retvect.push_back(nadr & 0xff);
      retvect.push_back((nadr >> 8) & 0xff);
      retvect.push_back(pnum);
      retvect.push_back(pcmd);
      retvect.push_back(hwpid & 0xff);
      retvect.push_back((hwpid >> 8) & 0xff);

      if (Value *val = Pointer("/rdata").Get(doc)) {
        uint8_t buf[DPA_MAX_DATA_LENGTH];
        int len = parseBinary(buf, val->GetString(), DPA_MAX_DATA_LENGTH);
        for (int i = 0; i < len; i++) {
          retvect.push_back(buf[i]);
        }
      }

      return retvect;
    }

    // nadr, hwpid not set for drivers
    std::string dpaResponseToRawHdpResponse(int& nadr, int& hwpid, const std::vector<uint8_t>& dpaResponse )
    {
      using namespace rapidjson;

      std::string rawHdpResponse;

      Document doc;

      if (dpaResponse.size() >= 8) {
        uint16_t nadr16 = 0, hwpid16 = 0;
        uint8_t pnum = 0, pcmd = 0, rcode = 0, dpaval = 0;
        std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;

        nadr = dpaResponse[0];
        nadr += dpaResponse[1] << 8;
        pnum = dpaResponse[2];
        pcmd = dpaResponse[3];
        hwpid = dpaResponse[4];
        hwpid += dpaResponse[5] << 8;
        rcode = dpaResponse[6];
        dpaval = dpaResponse[7];

        pnumStr = encodeHexaNum(pnum);
        pcmdStr = encodeHexaNum(pcmd);
        rcodeStr = encodeHexaNum(rcode);
        dpavalStr = encodeHexaNum(dpaval);

        //nadr, hwpid is not interesting for drivers
        Pointer("/pnum").Set(doc, pnumStr);
        Pointer("/pcmd").Set(doc, pcmdStr);
        Pointer("/rcode").Set(doc, rcodeStr);
        Pointer("/dpaval").Set(doc, rcodeStr);

        if (dpaResponse.size() > 8) {
          Pointer("/rdata").Set(doc, encodeBinary(dpaResponse.data() + 8, dpaResponse.size() - 8));
        }

        rawHdpResponse = JsonToStr(&doc);
      }

      return rawHdpResponse;
    }

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      using namespace rapidjson;

      Document allResponseDoc;
      std::unique_ptr<ComIqrfStandard> com(shape_new ComIqrfStandard(doc));

      std::string methodRequestName = msgType.m_possibleDriverFunction;
      std::string methodResponseName = msgType.m_possibleDriverFunction;
      methodRequestName += "_Request_req";
      methodResponseName += "_Response_rsp";

      // get req{} to be passed to _RequestObj driver func
      Value* reqObj = Pointer("/data/req").Get(doc);
      std::string reqObjStr = JsonToStr(reqObj);
        
      // get nadr, hwpid as driver ignore them
      int nadrReq = Pointer("/data/req/nAdr").Get(doc)->GetInt();
      int hwpidReq = Pointer("/data/req/hwpId").Get(doc)->GetInt();

      // call _RequestObj driver func
      // driver returns in rawHdpRequest format
      std::string rawHdpRequest;

      m_duk.call(methodRequestName, reqObjStr, rawHdpRequest);

      // convert from rawHdpRequest to dpaRequest and pass nadr and hwpid to be in dapaRequest (driver doesn't set them)
      std::vector<uint8_t> dpaRequest = rawHdpRequestToDpaRequest(nadrReq, hwpidReq, rawHdpRequest);

      // setDpaRequest as DpaMessage in com object 
      com->setDpaMessage(dpaRequest);

      // send to coordinator and wait for transaction result
      //-------------------
      auto trn = m_iIqrfDpaService->executeDpaTransaction(com->getDpaRequest());
      auto res = trn->get();
      //-------------------

      // get dpaResponse data
      const uint8_t *buf = res->getResponse().DpaPacket().Buffer;
      int sz = res->getResponse().GetLength();
      std::vector<uint8_t> dpaResponse(buf, buf + sz);

      // nadr, hwpid not set for drivers, so extract them for later use
      std::string rawHdpResponse;
      int nadrRes, hwpidRes;
      // get rawHdpResponse in text form
      rawHdpResponse = dpaResponseToRawHdpResponse(nadrRes, hwpidRes, dpaResponse);

      // call _RequestObj driver func
      // _ResponseObj driver func returns in rsp{} in text form
      std::string rspObjStr;
      m_duk.call(methodResponseName, rawHdpResponse, rspObjStr);
        
      // get json from its text representation
      Document rspObj;
      rspObj.Parse(rspObjStr);

      // set nadr, hwpid
      Pointer("/nAdr").Set(rspObj, nadrRes);
      Pointer("/hwpId").Set(rspObj, hwpidRes);

      { //debug
        std::string str = JsonToStr(&rspObj);
        TRC_DEBUG(str);
      }

      com->setPayload(std::move(rspObj));
      com->createResponse(allResponseDoc, *res);

      // TODO solve error if not methods in cache
      //com->createResponse(allResponseDoc, *res);

      //update message type - type is the same for request/response
      Pointer("/mType").Set(allResponseDoc, msgType.m_type);
      { //debug
        std::string str = JsonToStr(&allResponseDoc);
        TRC_DEBUG(str);
      }

      //TODO validate response in debug
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(allResponseDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApiIqrfStandard instance activate" << std::endl <<
        "******************************"
      );

      const std::map<int, const IJsCacheService::StdDriver*> scripts = m_iJsCacheService->getAllLatestDrivers();
      m_duk.init(scripts);

      m_iMessagingSplitterService->registerFilteredMsgHandler(m_filters,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
        {
          handleMsg(messagingId, msgType, std::move(doc));
        });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApiIqrfStandard instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

      m_duk.finit();
    
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
    }

    void attachInterface(IJsCacheService* iface)
    {
      m_iJsCacheService = iface;
    }

    void detachInterface(IJsCacheService* iface)
    {
      if (m_iJsCacheService == iface) {
        m_iJsCacheService = nullptr;
      }
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
  JsonDpaApiIqrfStandard::JsonDpaApiIqrfStandard()
  {
    m_imp = shape_new Imp();
  }

  JsonDpaApiIqrfStandard::~JsonDpaApiIqrfStandard()
  {
    delete m_imp;
  }

  void JsonDpaApiIqrfStandard::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonDpaApiIqrfStandard::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonDpaApiIqrfStandard::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonDpaApiIqrfStandard::attachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStandard::detachInterface(iqrf::IJsCacheService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStandard::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStandard::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStandard::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStandard::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStandard::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonDpaApiIqrfStandard::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
