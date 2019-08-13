#include "JsDriverFrc.h"
#include "JsDriverDali.h"
#include "ApiMsgIqrfStandard.h"
#include "IqrfSensorFrc.h"
//#include "IDpaTransactionResult2.h"
#include "JsonDpaApiIqrfStdSpec.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>

#include "iqrf__JsonDpaApiIqrfStdSpec.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApiIqrfStdSpec);

using namespace rapidjson;

namespace iqrf {

  class JsonDpaApiIqrfStdSpec::Imp
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
      "iqrfDali_Frc",
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

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document dc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      using namespace rapidjson;

      Document allResponseDoc;

      IqrfDaliFrc iqrfDaliFrc(dc);

      if (msgType.m_type == "iqrfDali_Frc") {
        try {
          dali::JsDriverFrc daliFrc(m_iJsRenderService, iqrfDaliFrc.getCommand(), iqrfDaliFrc.getSelectedNodes());

          daliFrc.processRequestDrv();

          auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

          std::unique_ptr<IDpaTransactionResult2> transResultFrc = exclusiveAccess->executeDpaTransaction(daliFrc.getFrcRequest())->get();
          daliFrc.setFrcDpaTransactionResult(std::move(transResultFrc));

          std::unique_ptr<IDpaTransactionResult2> transResultFrcExtra = exclusiveAccess->executeDpaTransaction(daliFrc.getFrcExtraRequest())->get();
          daliFrc.setFrcExtraDpaTransactionResult(std::move(transResultFrcExtra));

          daliFrc.processResponseDrv();

          auto items = daliFrc.getItems();

          iqrfDaliFrc.setPayload("/data/rsp/result", daliFrc.getResponseResultDoc());
          iqrfDaliFrc.setDpaTransactionResult(daliFrc.moveFrcDpaTransactionResult());
          iqrfDaliFrc.setDpaTransactionExtraResult(daliFrc.moveFrcExtraDpaTransactionResult());
          IDpaTransactionResult2::ErrorCode status = IDpaTransactionResult2::ErrorCode::TRN_OK;
          iqrfDaliFrc.setStatus(IDpaTransactionResult2::errorCode(status), status);
          iqrfDaliFrc.createResponse(allResponseDoc);
        }
        catch (HandleException & e) {
          //provide error response
          Document rDataError;
          rDataError.SetString(e.what(), rDataError.GetAllocator());
          iqrfDaliFrc.setPayload("/data/rsp/errorStr", std::move(rDataError));
          iqrfDaliFrc.setStatus(IDpaTransactionResult2::errorCode(e.getStatus()), e.getStatus());
          iqrfDaliFrc.createResponse(allResponseDoc);
        }
      }
      if (msgType.m_type == "iqrfSensor_Frc") {
        //try {
        //  dali::JsDriverFrc daliFrc(m_iJsRenderService, iqrfDaliFrc.getCommand(), iqrfDaliFrc.getSelectedNodes());

        //  daliFrc.processRequestDrv();

        //  auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

        //  std::unique_ptr<IDpaTransactionResult2> transResultFrc = exclusiveAccess->executeDpaTransaction(daliFrc.getFrcRequest())->get();
        //  daliFrc.setFrcDpaTransactionResult(std::move(transResultFrc));

        //  std::unique_ptr<IDpaTransactionResult2> transResultFrcExtra = exclusiveAccess->executeDpaTransaction(daliFrc.getFrcExtraRequest())->get();
        //  daliFrc.setFrcExtraDpaTransactionResult(std::move(transResultFrcExtra));

        //  daliFrc.processResponseDrv();

        //  auto items = daliFrc.getItems();

        //  iqrfDaliFrc.setPayload("/data/rsp/result", daliFrc.getResponseResultDoc());
        //  IDpaTransactionResult2::ErrorCode status = IDpaTransactionResult2::ErrorCode::TRN_OK;
        //  iqrfDaliFrc.setStatus(IDpaTransactionResult2::errorCode(status), status);
        //  iqrfDaliFrc.createResponse(allResponseDoc);
        //}
        //catch (HandleException & e) {
        //  //provide error response
        //  Document rDataError;
        //  rDataError.SetString(e.what(), rDataError.GetAllocator());
        //  iqrfDaliFrc.setPayload("/data/rsp/errorStr", std::move(rDataError));
        //  iqrfDaliFrc.setStatus(IDpaTransactionResult2::errorCode(e.getStatus()), e.getStatus());
        //  iqrfDaliFrc.createResponse(allResponseDoc);
        //}
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
        "JsonDpaApiIqrfStdSpec instance activate" << std::endl <<
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
        "JsonDpaApiIqrfStdSpec instance deactivate" << std::endl <<
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
  JsonDpaApiIqrfStdSpec::JsonDpaApiIqrfStdSpec()
  {
    m_imp = shape_new Imp();
  }

  JsonDpaApiIqrfStdSpec::~JsonDpaApiIqrfStdSpec()
  {
    delete m_imp;
  }

  void JsonDpaApiIqrfStdSpec::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonDpaApiIqrfStdSpec::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonDpaApiIqrfStdSpec::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonDpaApiIqrfStdSpec::attachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStdSpec::detachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStdSpec::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStdSpec::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStdSpec::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStdSpec::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiIqrfStdSpec::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonDpaApiIqrfStdSpec::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
