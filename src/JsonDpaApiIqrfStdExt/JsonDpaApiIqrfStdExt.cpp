/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

TRC_INIT_MODULE(iqrf::JsonDpaApiIqrfStdExt)

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
      using json = nlohmann::json;

      Document allResponseDoc;

      // solves api msg processing
      ApiMsgIqrfStandardFrc apiMsgIqrfStandardFrc(dc);

      try {
        // solves JsDriver processing
        JsDriverStandardFrcSolver jsDriverStandardFrcSolver(m_iJsRenderService, msgType.m_possibleDriverFunction,
          apiMsgIqrfStandardFrc.getRequestParamDoc(), apiMsgIqrfStandardFrc.getHwpid());

        // process *_Request
        jsDriverStandardFrcSolver.processRequestDrv();
        apiMsgIqrfStandardFrc.setDpaRequest(jsDriverStandardFrcSolver.getFrcRequest());

        auto exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
        int timeOut = apiMsgIqrfStandardFrc.getTimeout();
        // FRC transaction
        std::unique_ptr<IDpaTransactionResult2> transResultFrc = exclusiveAccess->executeDpaTransaction(jsDriverStandardFrcSolver.getFrcRequest(), timeOut)->get();
        jsDriverStandardFrcSolver.setFrcDpaTransactionResult(std::move(transResultFrc));

        if (apiMsgIqrfStandardFrc.getExtraResult()) {
          // FRC extra result transaction
          std::unique_ptr<IDpaTransactionResult2> transResultFrcExtra = exclusiveAccess->executeDpaTransaction(jsDriverStandardFrcSolver.getFrcExtraRequest())->get();
          jsDriverStandardFrcSolver.setFrcExtraDpaTransactionResult(std::move(transResultFrcExtra));
        }

        // process *_Response
        jsDriverStandardFrcSolver.processResponseDrv();

        json jsonDoc = json::parse(jsDriverStandardFrcSolver.getResponseResultStr());
        std::string arrayKey = apiMsgIqrfStandardFrc.getArrayKeyByMessageType(msgType.m_type);
        if (apiMsgIqrfStandardFrc.hasSelectedNodes()) {
          jsDriverStandardFrcSolver.filterSelectedNodes(
            jsonDoc,
            arrayKey,
            apiMsgIqrfStandardFrc.getSelectedNodes().size()
          );
        }
        if (apiMsgIqrfStandardFrc.getExtFormat()) {
          
          jsDriverStandardFrcSolver.convertToExtendedFormat(
            jsonDoc,
            arrayKey,
            apiMsgIqrfStandardFrc.getItemKeyByMessageType(msgType.m_type),
            apiMsgIqrfStandardFrc.getSelectedNodes()
          );

          if (m_iIqrfInfo && m_iIqrfInfo->getMidMetaDataToMessages()) {
            try {
              for (auto itr = jsonDoc[arrayKey].begin(); itr != jsonDoc[arrayKey].end(); ++itr) {
                uint8_t nadr = (*itr)["nAdr"].get<uint8_t>();
                if (m_iIqrfInfo && m_iIqrfInfo->getMidMetaDataToMessages()) {
                  std::string metadataStr = jutils::jsonToStr(m_iIqrfInfo->getNodeMetaData(nadr));
                  (*itr)["metaData"] = json::parse(metadataStr);
                }
              }
            } catch (const std::exception &e) {
              CATCH_EXC_TRC_WAR(std::exception, e, "Cannot annotate by metadata");
            }
          }
        }

        std::string jsonDocStr = jsonDoc.dump();
        Document doc;
        doc.Parse(jsonDocStr);
        apiMsgIqrfStandardFrc.setPayload("/data/rsp/result", doc);

        apiMsgIqrfStandardFrc.setDpaTransactionResult(jsDriverStandardFrcSolver.moveFrcDpaTransactionResult());
        apiMsgIqrfStandardFrc.setDpaTransactionExtraResult(jsDriverStandardFrcSolver.moveFrcExtraDpaTransactionResult());
        IDpaTransactionResult2::ErrorCode status = IDpaTransactionResult2::ErrorCode::TRN_OK;
        apiMsgIqrfStandardFrc.setStatus(IDpaTransactionResult2::errorCode(status), status);
        apiMsgIqrfStandardFrc.createResponse(allResponseDoc);
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
