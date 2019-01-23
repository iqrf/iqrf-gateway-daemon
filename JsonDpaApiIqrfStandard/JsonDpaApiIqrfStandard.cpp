#define IMessagingSplitterService_EXPORTS

#include "ComIqrfStandard.h"
#include "IDpaTransactionResult2.h"
#include "JsonDpaApiIqrfStandard.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
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
  class FakeTransactionResult : public IDpaTransactionResult2
  {
  public:
    int getErrorCode() const override { return m_errCode; }
    void overrideErrorCode(IDpaTransactionResult2::ErrorCode err) override { m_errCode = err; }
    std::string getErrorString() const override { return "BAD_REQUEST"; }

    virtual const DpaMessage& getRequest() const override { return m_fake; }
    virtual const DpaMessage& getConfirmation() const override { return m_fake; }
    virtual const DpaMessage& getResponse() const override { return m_fake; }
    virtual const std::chrono::time_point<std::chrono::system_clock>& getRequestTs() const override { return m_now; }
    virtual const std::chrono::time_point<std::chrono::system_clock>& getConfirmationTs() const override { return m_now; }
    virtual const std::chrono::time_point<std::chrono::system_clock>& getResponseTs() const override { return m_now; }
    virtual bool isConfirmed() const override { return false; }
    virtual bool isResponded() const override { return false; }
    virtual ~FakeTransactionResult() {};
  private:
    DpaMessage m_fake;
    IDpaTransactionResult2::ErrorCode m_errCode = TRN_ERROR_BAD_REQUEST;
    std::chrono::time_point<std::chrono::system_clock> m_now;
  };

  class JsonDpaApiIqrfStandard::Imp
  {
  private:

    IMetaDataApi* m_iMetaDataApi = nullptr;
    IJsRenderService* m_iJsRenderService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    //just to be able to abort
    std::mutex m_iDpaTransactionMtx;
    std::shared_ptr<IDpaTransaction2> m_iDpaTransaction;

    // TODO from cfg
    std::vector<std::string> m_filters =
    {
      "iqrfEmbed",
      "iqrfLight",
      "iqrfSensor",
      "iqrfBinaryoutput"
    };

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
    //TODO return directly DpaMessage to avoid later conversion vector -> DpaMessage
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

    // nadr, hwpid, rcode not set for drivers
    //TODO rewrite with const DpaMessage& dpaResponse to avoid previous conversion DpaMessage -> vector
    std::string dpaResponseToRawHdpResponse(int& nadr, int& hwpid, int& rcode, const std::vector<uint8_t>& dpaResponse)
    {
      using namespace rapidjson;

      std::string rawHdpResponse;

      Document doc;

      if (dpaResponse.size() >= 8) {
        uint16_t nadr16 = 0, hwpid16 = 0;
        uint8_t pnum = 0, pcmd = 0, rcode8 = 0, dpaval = 0;
        std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;

        nadr = dpaResponse[0];
        nadr += dpaResponse[1] << 8;
        pnum = dpaResponse[2];
        pcmd = dpaResponse[3];
        hwpid = dpaResponse[4];
        hwpid += dpaResponse[5] << 8;
        rcode8 = dpaResponse[6];
        rcode = rcode8;
        dpaval = dpaResponse[7];

        pnumStr = encodeHexaNum(pnum);
        pcmdStr = encodeHexaNum(pcmd);
        rcodeStr = encodeHexaNum(rcode8);
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

      if (m_iMetaDataApi) {
        if (m_iMetaDataApi->iSmetaDataToMessages()) {
          com->setMetaData(m_iMetaDataApi->getMetaData(com->getNadr()));
        }
      }

      std::string methodRequestName = msgType.m_possibleDriverFunction;
      std::string methodResponseName = msgType.m_possibleDriverFunction;
      methodRequestName += "_Request_req";
      methodResponseName += "_Response_rsp";

      // call request driver func, it returns rawHdpRequest format in text form
      std::string rawHdpRequest;
      std::string errStrReq;
      bool driverRequestError = false;
      try {
        m_iJsRenderService->call(methodRequestName, com->getParamAsString(), rawHdpRequest);
      }
      catch (std::exception &e) {
        //request driver func error
        errStrReq = e.what();
        driverRequestError = true;
      }

      if (driverRequestError) {
        //provide error response
        Document rDataError;
        rDataError.SetString(errStrReq, rDataError.GetAllocator());
        com->setPayload("/data/rsp/errorStr", rDataError, true);
        FakeTransactionResult fr;
        com->setStatus(fr.getErrorString(), fr.getErrorCode());
        com->createResponse(allResponseDoc, fr);
      }
      else {
        TRC_DEBUG(PAR(rawHdpRequest));
        // convert from rawHdpRequest to dpaRequest and pass nadr and hwpid to be in dapaRequest (driver doesn't set them)
        int hwpidReq = com->getHwpid();
        std::vector<uint8_t> dpaRequest = rawHdpRequestToDpaRequest(com->getNadr(), hwpidReq < 0 ? 0xffff : hwpidReq, rawHdpRequest);

        // setDpaRequest as DpaMessage in com object 
        com->setDpaMessage(dpaRequest);

        // send to coordinator and wait for transaction result
        {
          std::lock_guard<std::mutex> lck(m_iDpaTransactionMtx);
          m_iDpaTransaction = m_iIqrfDpaService->executeDpaTransaction(com->getDpaRequest(), com->getTimeout());
        }
        auto res = m_iDpaTransaction->get();


        //process response
        int nadrRes = com->getNadr();
        int hwpidRes = 0;
        int rcode = -1;

        if (res->isResponded()) {
          //we have some response
          const uint8_t *buf = res->getResponse().DpaPacket().Buffer;
          int sz = res->getResponse().GetLength();
          std::vector<uint8_t> dpaResponse(buf, buf + sz);

          // get rawHdpResponse in text form
          std::string rawHdpResponse;
          rawHdpResponse = dpaResponseToRawHdpResponse(nadrRes, hwpidRes, rcode, dpaResponse);
          TRC_DEBUG(PAR(rawHdpResponse))

          if (0 == rcode) {
            // call response driver func, it returns rsp{} in text form
            std::string rspObjStr;
            std::string errStrRes;
            bool driverResponseError = false;
            try {
              m_iJsRenderService->call(methodResponseName, rawHdpResponse, rspObjStr);
            }
            catch (std::exception &e) {
              //response driver func error
              errStrRes = e.what();
              driverResponseError = true;
            }

            if (driverResponseError) {
              //provide error response
              Document rDataError;
              rDataError.SetString(errStrRes.c_str(), rDataError.GetAllocator());
              com->setPayload("/data/rsp/errorStr", rDataError, true);
              res->overrideErrorCode(IDpaTransactionResult2::ErrorCode::TRN_ERROR_BAD_RESPONSE);
              com->setStatus(res->getErrorString(), res->getErrorCode());
              com->createResponse(allResponseDoc, *res);
            }
            else {
              // get json from its text representation
              Document rspObj;
              rspObj.Parse(rspObjStr);
              TRC_DEBUG("result object: " << std::endl << JsonToStr(&rspObj));
              com->setPayload("/data/rsp/result", rspObj, false);
              com->setStatus(res->getErrorString(), res->getErrorCode());
              com->createResponse(allResponseDoc, *res);
            }
          }
          else {
            Document rDataError;
            rDataError.SetString("rcode error", rDataError.GetAllocator());
            com->setPayload("/data/rsp/errorStr", rDataError, true);
            com->setStatus(res->getErrorString(), res->getErrorCode());
            com->createResponse(allResponseDoc, *res);
          }
        }
        else {
          if (res->getErrorCode() != 0) {
            Document rDataError;
            rDataError.SetString("rcode error", rDataError.GetAllocator());
            com->setPayload("/data/rsp/errorStr", rDataError, true);
            com->setStatus(res->getErrorString(), res->getErrorCode());
            com->createResponse(allResponseDoc, *res);
          }
          else {
            //no response but not considered as an error
            Document rspObj;
            Pointer("/response").Set(rspObj, "unrequired");
            com->setPayload("/data/rsp/result", rspObj, false);
            com->setStatus(res->getErrorString(), res->getErrorCode());
            com->createResponse(allResponseDoc, *res);
          }
        }
      }
      TRC_DEBUG("response object: " << std::endl << JsonToStr(&allResponseDoc));

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

      {
        std::lock_guard<std::mutex> lck(m_iDpaTransactionMtx);
        if (m_iDpaTransaction) {
          m_iDpaTransaction->abort();
        }
      }

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
    }

    void attachInterface(IMetaDataApi* iface)
    {
      m_iMetaDataApi = iface;
    }

    void detachInterface(IMetaDataApi* iface)
    {
      if (m_iMetaDataApi == iface) {
        m_iMetaDataApi = nullptr;
      }
    }

    void attachInterface(IJsRenderService* iface)
    {
      m_iJsRenderService = iface;
    }

    void detachInterface(IJsRenderService* iface)
    {
      if (m_iJsRenderService == iface) {
        m_iJsRenderService = nullptr;
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

  void JsonDpaApiIqrfStandard::attachInterface(iqrf::IMetaDataApi* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStandard::detachInterface(iqrf::IMetaDataApi* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStandard::attachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStandard::detachInterface(iqrf::IJsRenderService* iface)
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
