#include "ComIqrfStandardExt.h"
#include "IDpaTransactionResult2.h"
#include "JsonDpaApiIqrfStdExt.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>

#include "iqrf__JsonDpaApiIqrfStdExt.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApiIqrfStdExt);

using namespace rapidjson;

namespace iqrf {

  class JsonDpaApiIqrfStdExt::Imp
  {
  private:

    iqrf::IJsRenderService* m_iJsRenderService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    //just to be able to abort
    std::mutex m_iDpaTransactionMtx;
    std::shared_ptr<IDpaTransaction2> m_iDpaTransaction;

    // TODO from cfg
    std::vector<std::string> m_filters =
    {
      "iqrfSensor_Frc",
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

    struct StandardDriverRequestResult
    {
      bool success;
      DpaMessage dpaRequest;
      std::string errStr;
    };

    //TODO not used here, ready for redesign JsonDpaApiIqrfStandard to publih JsDriver interface
    StandardDriverRequestResult callDriverRequest(const std::string & methodRequestName, const std::string & params, uint16_t nadr, uint16_t hwpid)
    {
      TRC_FUNCTION_ENTER(PAR(methodRequestName) << PAR(params) << PAR(nadr) << PAR(hwpid));

      GenericDriverRequestResult gresult = callDriverRequest(methodRequestName, params);
      StandardDriverRequestResult result;
      result.success = gresult.success;
      result.errStr = gresult.errStr;

      if (result.success) {
        RawHdpRequest rawHdpRequest(gresult.resultStr, nadr, hwpid);
        result.dpaRequest = rawHdpRequest.getDpaRequest();
      }

      TRC_FUNCTION_LEAVE(PAR(result.success) << PAR(result.errStr) << MEM_HEX(result.dpaRequest.DpaPacket().Buffer, result.dpaRequest.GetLength()));
      return result;
    }

    struct GenericDriverRequestResult
    {
      bool success;
      std::string resultStr;
      std::string errStr;
    };

    GenericDriverRequestResult callDriverRequest(const std::string & methodRequestName, const std::string & params)
    {
      TRC_FUNCTION_ENTER(PAR(methodRequestName) << PAR(params));

      GenericDriverRequestResult result;
      result.success = true;

      // call requestdriver func, it returns result params in text form
      try {
        m_iJsRenderService->call(methodRequestName, params, result.resultStr);
      }
      catch (std::exception &e) {
        result.errStr = e.what();
        result.success = false;
      }

      TRC_FUNCTION_LEAVE(PAR(result.success) << PAR(result.errStr) << PAR(result.resultStr));
      return result;
    }

    struct StandardDriverResponseResult
    {
      bool success;
      int nadr;
      int hwpid;
      int rcode;
      std::string rspObjStr;
      std::string errStr;
    };

    StandardDriverResponseResult callDriverResponse(const std::string & methodResponseName, const DpaMessage dpaMessage)
    {
      TRC_FUNCTION_ENTER(PAR(methodResponseName));

      StandardDriverResponseResult result;
      result.success = true;

      RawHdpResponse rawHdpResponse(dpaMessage);

      if (0 == result.rcode) {
        GenericDriverResponseResult gresult = callDriverResponse(methodResponseName, rawHdpResponse.getString());
        result.success = gresult.success;
        result.errStr = gresult.errStr;
        result.rspObjStr = gresult.rspObjStr;
      }
      else {
        result.errStr = "rcode error";
        result.success = false;
      }

      TRC_FUNCTION_LEAVE(PAR(result.success) << PAR(result.errStr) << PAR(result.rspObjStr));
      return result;
    }

    struct GenericDriverResponseResult
    {
      bool success;
      std::string rspObjStr;
      std::string errStr;
    };

    GenericDriverResponseResult callDriverResponse(const std::string & methodResponseName, const std::string& param)
    {
      TRC_FUNCTION_ENTER(PAR(methodResponseName) << PAR(param));

      GenericDriverResponseResult result;
      result.success = true;

      // call response driver func, it returns rsp{} in text form
      try {
        m_iJsRenderService->call(methodResponseName, param, result.rspObjStr);
      }
      catch (std::exception &e) {
        result.errStr = e.what();
        result.success = false;
      }

      TRC_FUNCTION_LEAVE(PAR(result.success) << PAR(result.errStr) << PAR(result.rspObjStr));
      return result;
    }

    // aux exception to handle error situations
    class HandleException : public std::logic_error
    {
    public:
      HandleException() = delete;
      HandleException(const std::string& errStr, int status)
        :std::logic_error(errStr.c_str())
        ,m_status(status)
      {
      }
      int getStatus() const { return m_status; }
    private:
      int m_status = 1;
    };

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      using namespace rapidjson;

      Document allResponseDoc;

      std::string methodRequestName = msgType.m_possibleDriverFunction;
      std::string methodResponseName = msgType.m_possibleDriverFunction;
      methodRequestName += "_Request_req";
      methodResponseName += "_Response_rsp";

      IqrfSensorFrc iqrfSensorFrc(doc);

      try {
        //if (msgType.m_type == "iqrfSensor_Frc") {
        //IqrfSensorFrc iqrfSensorFrc(doc);

        // call request driver func with param as string
        GenericDriverRequestResult requestRes =
          callDriverRequest(methodRequestName, iqrfSensorFrc.getParamAsString());

        if (!requestRes.success) {
          TRC_WARNING(PAR(methodRequestName) << " error " << PAR(requestRes.errStr));
          throw HandleException(requestRes.errStr, IDpaTransactionResult2::ErrorCode::TRN_ERROR_BAD_REQUEST);
        }

        // parse result
        Document doc;
        doc.Parse(requestRes.resultStr);

        // get FRC request
        Value *val0 = Pointer("/retpars/0").Get(doc);
        if (!val0 || !val0->IsObject()) {
          const char* errstr = "invalid format from JS driver /retpars/0";
          TRC_WARNING(PAR(methodRequestName) << " error " << PAR(errstr));
          throw HandleException(errstr, IDpaTransactionResult2::ErrorCode::TRN_ERROR_BAD_REQUEST);
        }
        
        RawHdpRequest rawHdpRequest(*val0, iqrfSensorFrc.getNadr(), iqrfSensorFrc.getHwpid());
        iqrfSensorFrc.setDpaRequest(rawHdpRequest.getDpaRequest());
        //iqrfSensorFrc.setDpaRequest(RawHdpRequest(*val0, iqrfSensorFrc.getNadr(), iqrfSensorFrc.getHwpid()).getDpaRequest());

        if (iqrfSensorFrc.getExtraResult()) {
          // get FRC extra request
          Value *val1 = Pointer("/retpars/1").Get(doc);
          if (!val1 && !val1->IsObject()) {
            const char* errstr = "invalid format from JS driver /retpars/1";
            TRC_WARNING(PAR(methodRequestName) << " error " << PAR(errstr));
            throw HandleException(errstr, IDpaTransactionResult2::ErrorCode::TRN_ERROR_BAD_REQUEST);
          }
          iqrfSensorFrc.setDpaRequestExtra(RawHdpRequest(*val1, iqrfSensorFrc.getNadr(), iqrfSensorFrc.getHwpid()).getDpaRequest());
        }

        std::unique_ptr<IIqrfDpaService::ExclusiveAccess> exclusiveAccess;
        try {
          exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
        }
        catch (std::exception &e) {
          const char* errstr = e.what();
          TRC_WARNING(PAR(methodRequestName) << " error " << PAR(errstr));
          throw HandleException(errstr, IDpaTransactionResult2::ErrorCode::TRN_ERROR_IFACE_EXCLUSIVE_ACCESS);
        }

        // send to coordinator DpaRequest and wait for transaction result
        {
          TRC_DEBUG("Sending FRC request");
          std::lock_guard<std::mutex> lck(m_iDpaTransactionMtx);
          //m_iDpaTransaction = m_iIqrfDpaService->executeDpaTransaction(iqrfSensorFrc.getDpaRequest(), iqrfSensorFrc.getTimeout());
          m_iDpaTransaction = exclusiveAccess->executeDpaTransaction(iqrfSensorFrc.getDpaRequest(), iqrfSensorFrc.getTimeout());
        }
        auto resultDpaRequest = m_iDpaTransaction->get();

        // get necessary data from DpaRequest result and move
        bool isResponded = resultDpaRequest->isResponded();
        int errCode = resultDpaRequest->getErrorCode();
        iqrfSensorFrc.setDpaTransactionResult(std::move(resultDpaRequest));
        
        if (!isResponded || errCode != IDpaTransactionResult2::ErrorCode::TRN_OK) {
          const char* errstr = "no FRC response";
          TRC_WARNING("Error: " << PAR(errstr));
          throw HandleException(errstr, errCode);
        }

        if (iqrfSensorFrc.getExtraResult()) {
          // send to coordinator DpaRequestExtra wait for transaction result
          {
            TRC_DEBUG("Sending FRC extra request");
            std::lock_guard<std::mutex> lck(m_iDpaTransactionMtx);
            //m_iDpaTransaction = m_iIqrfDpaService->executeDpaTransaction(iqrfSensorFrc.getDpaRequestExtra(), iqrfSensorFrc.getTimeout());
            m_iDpaTransaction = exclusiveAccess->executeDpaTransaction(iqrfSensorFrc.getDpaRequestExtra(), iqrfSensorFrc.getTimeout());
          }
          auto resultDpaRequestExtra = m_iDpaTransaction->get();

          // get necessary data from DpaRequestExtra result and move
          isResponded = resultDpaRequestExtra->isResponded();
          errCode = resultDpaRequestExtra->getErrorCode();
          iqrfSensorFrc.setDpaTransactionExtraResult(std::move(resultDpaRequestExtra));

          if (!isResponded || errCode != IDpaTransactionResult2::ErrorCode::TRN_OK) {
            const char* errstr = "no FRC Extra response";
            TRC_WARNING("Error: " << PAR(errstr));
            throw HandleException(errstr, errCode);
          }
        }

        exclusiveAccess.reset();

        // prepare params for driver
        Document paramDoc;

        Pointer("/sensorType").Set(paramDoc, iqrfSensorFrc.getSensorType());
        Pointer("/frcCommand").Set(paramDoc, iqrfSensorFrc.getFrcCommand());
        if (const DpaMessage* r = iqrfSensorFrc.getDpaResponse()) {
          Pointer("/responseFrcSend").Set(paramDoc, RawHdpResponse(*r).getDocument());
        }
        if (const DpaMessage* r = iqrfSensorFrc.getDpaResponseExtra()) {
          Pointer("/responseFrcExtraResult").Set(paramDoc, RawHdpResponse(*r).getDocument());
        }
        
        //TODO parse request
        
        Pointer("/frcSendRequest").Set(paramDoc, rawHdpRequest.getDocument());

        StringBuffer buffer;
        PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        paramDoc.Accept(writer);
        std::string param = buffer.GetString();
        TRC_DEBUG(param);

        GenericDriverResponseResult responseResult = callDriverResponse(methodResponseName, param);
        if (!responseResult.success) {
          TRC_WARNING(PAR(methodRequestName) << " error " << PAR(responseResult.errStr));
          throw HandleException(responseResult.errStr, IDpaTransactionResult2::ErrorCode::TRN_ERROR_BAD_RESPONSE);
        }

        // get json from its text representation
        Document rspObj;
        rspObj.Parse(responseResult.rspObjStr);
        TRC_DEBUG("result object: " << std::endl << JsonToStr(&rspObj));
        iqrfSensorFrc.setPayload("/data/rsp/result", std::move(rspObj));
        IDpaTransactionResult2::ErrorCode status = IDpaTransactionResult2::ErrorCode::TRN_OK;
        iqrfSensorFrc.setStatus(IDpaTransactionResult2::errorCode(status), status);
        iqrfSensorFrc.createResponse(allResponseDoc);
      }
      catch (HandleException & e) {
        //provide error response
        Document rDataError;
        rDataError.SetString(e.what(), rDataError.GetAllocator());
        iqrfSensorFrc.setPayload("/data/rsp/errorStr", std::move(rDataError));
        iqrfSensorFrc.setStatus(IDpaTransactionResult2::errorCode(e.getStatus()), e.getStatus());
        iqrfSensorFrc.createResponse(allResponseDoc);
      }

      TRC_DEBUG("response object: " << std::endl << JsonToStr(&allResponseDoc));
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(allResponseDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApiIqrfStdExt instance activate" << std::endl <<
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
        "JsonDpaApiIqrfStdExt instance deactivate" << std::endl <<
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
      (void)props; //silence -Wunused-parameter
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
  JsonDpaApiIqrfStdExt::JsonDpaApiIqrfStdExt()
  {
    m_imp = shape_new Imp();
  }

  JsonDpaApiIqrfStdExt::~JsonDpaApiIqrfStdExt()
  {
    delete m_imp;
  }

  void JsonDpaApiIqrfStdExt::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonDpaApiIqrfStdExt::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonDpaApiIqrfStdExt::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonDpaApiIqrfStdExt::attachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStdExt::detachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStdExt::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStdExt::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStdExt::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStdExt::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStdExt::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonDpaApiIqrfStdExt::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
