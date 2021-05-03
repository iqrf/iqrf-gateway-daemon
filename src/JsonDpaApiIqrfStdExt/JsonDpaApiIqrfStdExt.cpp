#include "ApiMsgIqrfStandardFrc.h"
#include "JsDriverStandardFrcSolver.h"
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

    IIqrfInfo* m_iIqrfInfo = nullptr;
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
      "iqrfSensor_Frc"
    };

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document dc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      using namespace rapidjson;

      Document allResponseDoc;

      // solves api msg processing
      ApiMsgIqrfStandardFrc apiMsgIqrfStandardFrc(dc);

      try {
        // solves JsDriver processing
        JsDriverStandardFrcSolver jsDriverStandardFrcSolver(m_iJsRenderService, msgType.m_possibleDriverFunction,
          apiMsgIqrfStandardFrc.getRequestParamDoc(), apiMsgIqrfStandardFrc.getHwpid());

        // process *_Request
        jsDriverStandardFrcSolver.processRequestDrv();

        auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();

        // FRC transaction
        std::unique_ptr<IDpaTransactionResult2> transResultFrc = exclusiveAccess->executeDpaTransaction(jsDriverStandardFrcSolver.getFrcRequest())->get();
        jsDriverStandardFrcSolver.setFrcDpaTransactionResult(std::move(transResultFrc));

        if (apiMsgIqrfStandardFrc.getExtraResult()) {
          // FRC extra result transaction
          std::unique_ptr<IDpaTransactionResult2> transResultFrcExtra = exclusiveAccess->executeDpaTransaction(jsDriverStandardFrcSolver.getFrcExtraRequest())->get();
          jsDriverStandardFrcSolver.setFrcExtraDpaTransactionResult(std::move(transResultFrcExtra));
        }

        // process *_Response
        jsDriverStandardFrcSolver.processResponseDrv();

        // finalize api response
        if (!apiMsgIqrfStandardFrc.getExtFormat()) {
          apiMsgIqrfStandardFrc.setPayload("/data/rsp/result", jsDriverStandardFrcSolver.getResponseResultDoc());
        }
        else {
          std::string arrayKey, itemKey;

          // TODO maybe better to virtualize getExtFormat() 
          if (msgType.m_type == "iqrfDali_Frc") {
            arrayKey = "/answers";
            itemKey = "/answer";
          }
          else if (msgType.m_type == "iqrfSensor_Frc") {
            arrayKey = "/sensors";
            itemKey = "/sensor";
          }
          else {
            THROW_EXC_TRC_WAR(std::logic_error, "Unexpected: " << NAME_PAR(messageType, msgType.m_type));
          }

          rapidjson::Document doc = jsDriverStandardFrcSolver.getExtFormat(arrayKey, itemKey);

          for (Value * senVal = doc.Begin(); senVal != doc.End(); senVal++) {
            Value *nadrVal = Pointer("/nAdr").Get(*senVal);
            if (!(nadrVal && nadrVal->IsInt())) {
              THROW_EXC_TRC_WAR(std::logic_error, "Expected: .../nAdr of type integer");
            }

            int nadr = nadrVal->GetInt();

            try {
              // db metadata
              if (m_iIqrfInfo && m_iIqrfInfo->getMidMetaDataToMessages()) {
                // anotate with metada
                Pointer("/metaData").Set(*senVal, m_iIqrfInfo->getNodeMetaData(nadr), doc.GetAllocator());
              }
            }
            catch (std::exception & e) {
              CATCH_EXC_TRC_WAR(std::exception, e, "Cannot annotate by metadata");
            }
          }

          //TODO right key here
          std::string fullArrayKey = "/data/rsp/result";
          fullArrayKey += arrayKey;
          apiMsgIqrfStandardFrc.setPayload(fullArrayKey, doc);
        }
        apiMsgIqrfStandardFrc.setDpaTransactionResult(jsDriverStandardFrcSolver.moveFrcDpaTransactionResult());
        apiMsgIqrfStandardFrc.setDpaTransactionExtraResult(jsDriverStandardFrcSolver.moveFrcExtraDpaTransactionResult());
        IDpaTransactionResult2::ErrorCode status = IDpaTransactionResult2::ErrorCode::TRN_OK;
        apiMsgIqrfStandardFrc.setStatus(IDpaTransactionResult2::errorCode(status), status);
        apiMsgIqrfStandardFrc.createResponse(allResponseDoc);

        //{ //TODO debug  only
        //  rapidjson::StringBuffer buffer;
        //  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        //  allResponseDoc.Accept(writer);
        //  std::string allResponseDocStr = buffer.GetString();
        //}

      }
      catch (std::exception & e) {
        //provide error response
        Document rDataError;
        rDataError.SetString(e.what(), rDataError.GetAllocator());
        apiMsgIqrfStandardFrc.setPayload("/data/rsp/errorStr", std::move(rDataError));
        //apiMsgIqrfStandardFrc.setStatus(IDpaTransactionResult2::errorCode(e.getStatus()), e.getStatus());
        apiMsgIqrfStandardFrc.setStatus(
          IDpaTransactionResult2::errorCode(IDpaTransactionResult2::ErrorCode::TRN_ERROR_FAIL), IDpaTransactionResult2::ErrorCode::TRN_ERROR_FAIL);
        apiMsgIqrfStandardFrc.createResponse(allResponseDoc);
      }

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
      auto thr = pthread_self();
      pthread_setname_np(thr, "igdDpaApiIqrfStdExt");

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

    void attachInterface(IIqrfInfo* iface)
    {
      m_iIqrfInfo = iface;
    }

    void detachInterface(IIqrfInfo* iface)
    {
      if (m_iIqrfInfo == iface) {
        m_iIqrfInfo = nullptr;
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

  void JsonDpaApiIqrfStdExt::attachInterface(IIqrfInfo* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiIqrfStdExt::detachInterface(IIqrfInfo* iface)
  {
    m_imp->detachInterface(iface);
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
