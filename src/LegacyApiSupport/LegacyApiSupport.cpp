/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

#define ILegacyApiSupport_EXPORTS

#include "IDpaHandler2.h"
#include "DpaTask.h"
#include "LegacyApiSupport.h"
#include "JsonSerializer.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "Trace.h"

#include "iqrf__LegacyApiSupport.hxx"

TRC_INIT_MODULE(iqrf::LegacyApiSupport);

namespace iqrf {
  LegacyApiSupport::LegacyApiSupport()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  LegacyApiSupport::~LegacyApiSupport()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  void LegacyApiSupport::handleMsgFromMessaging(const std::string & messagingId, const std::basic_string<uint8_t> & msg)
  {
    TRC_INFORMATION(std::endl << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl <<
      "Message to process: " << std::endl << MEM_HEX_CHAR(msg.data(), msg.size()));

    //to encode output message
    std::ostringstream os;

    //get input message
    std::string msgs((const char*)msg.data(), msg.size());
    std::istringstream is(msgs);

    std::unique_ptr<DpaTask> dpaTask;
    std::string command;

    //parse
    bool handled = false;
    std::string ctype;
    std::string lastError = "Unknown ctype";
    //for (auto ser : m_serializerVect) {
    ctype = m_serializer.parseCategory(msgs);
    if (ctype == CAT_DPA_STR) {
      dpaTask = m_serializer.parseRequest(msgs);
      if (dpaTask) {
        //DpaTransactionTask trans(*dpaTask);
        try {

          std::shared_ptr<IDpaTransaction2> dt = m_dpa->executeDpaTransaction(dpaTask->getRequest(), dpaTask->getTimeout());
          std::unique_ptr<IDpaTransactionResult2> dtr = dt->get();

          //TODO fill taskDpa according result
          dpaTask->timestampRequest(dtr->getRequestTs());
          if (dtr->getConfirmation().GetLength() > 0) {
            dpaTask->handleConfirmation(dtr->getConfirmation());
            dpaTask->timestampConfirmation(dtr->getConfirmationTs());
          }
          if (dtr->getResponse().GetLength() > 0) {
            dpaTask->handleResponse(dtr->getResponse());
            dpaTask->timestampResponse(dtr->getResponseTs());
          }
          //workaround for V2/V1 support
          std::string errStr;
          if (dtr->getErrorCode() != 0) {
            errStr = dtr->getErrorString();
          }
          else {
            errStr = "STATUS_NO_ERROR";
          }
          os << dpaTask->encodeResponse(errStr);
        }
        catch (std::exception &e) {
          CATCH_EXC_TRC_WAR(std::exception, e, " during message processing");
        }

        //TODO
        //just stupid hack for test async - remove it
        ///////
        //handleAsyncDpaMessage(dpaTask->getResponse());
        //handleAsyncDpaMessage(dpaTask->getRequest());
        ///////
        handled = true;
      }
      lastError = m_serializer.getLastError();
      //break;
    }
    else if (ctype == CAT_CONF_STR) {
      command = m_serializer.parseConfig(msgs);
      if (!command.empty()) {
        //TODO
        //std::string response = m_daemon->doCommand(command);
        //lastError = m_serializer->getLastError();
        //os << m_serializer->encodeConfig(msgs, response);
        handled = true;
      }
      lastError = m_serializer.getLastError();
      //break;
    }
    //}

    if (!handled) {
      os << "PARSE ERROR: " << PAR(ctype) << PAR(lastError);
    }

    //TRC_INFORMATION("Response to send: " << std::endl << MEM_HEX_CHAR(msgu.data(), msgu.size()) << std::endl <<
    //  ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl);

    rapidjson::Document doc;
    doc.Parse(os.str().c_str());
    m_iMessagingSplitterService->sendMessage(messagingId, std::move(doc));

  }

  void LegacyApiSupport::handleAsyncDpaMessage(const DpaMessage& dpaMessage)
  {
    TRC_FUNCTION_ENTER("");
    std::string sr = m_serializer.encodeAsyncAsDpaRaw(dpaMessage);
    TRC_INFORMATION(std::endl << "<<<<< ASYNCHRONOUS <<<<<<<<<<<<<<<" << std::endl <<
      "Asynchronous message to send: " << std::endl << MEM_HEX(sr.data(), sr.size()) << std::endl <<
      ">>>>> ASYNCHRONOUS >>>>>>>>>>>>>>>" << std::endl);
    std::basic_string<uint8_t> msgu((unsigned char*)sr.data(), sr.size());

    //m_messaging->sendMessage(msgu);
    //TODO
    //m_iMessagingSplitterService->sendMessage(msgu);

    TRC_FUNCTION_LEAVE("");
  }

  void LegacyApiSupport::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "LegacyApiSupport instance activate" << std::endl <<
      "******************************"
    );

    props->getMemberAsString("instance", m_name);

    //if (m_asyncDpaMessage) {
    //  TRC_INFORMATION("Set AsyncDpaMessageHandler :" << PAR(m_name));
    //  m_dpa->registerAsyncMessageHandler(m_name, [&](const DpaMessage& dpaMessage) {
    //    handleAsyncDpaMessage(dpaMessage);
    //  });
    //}

    m_iMessagingSplitterService->registerFilteredMsgHandler(m_filters,
      [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
    {
      (void)msgType; //silence -Wunused-parameter
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      std::string msg = buffer.GetString();
      std::basic_string<uint8_t> umsg((uint8_t*)msg.data(), msg.size());
      handleMsgFromMessaging(messagingId, umsg);
    });

    TRC_FUNCTION_LEAVE("")
  }

  void LegacyApiSupport::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "LegacyApiSupport instance deactivate" << std::endl <<
      "******************************"
    );

    m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);

    TRC_FUNCTION_LEAVE("")
  }

  void LegacyApiSupport::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void LegacyApiSupport::attachInterface(iqrf::IMessagingSplitterService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_iMessagingSplitterService = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void LegacyApiSupport::detachInterface(iqrf::IMessagingSplitterService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_iMessagingSplitterService == iface) {
      m_iMessagingSplitterService = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  //void LegacyApiSupport::attachInterface(iqrf::IJsonSerializerService* iface)
  //{
  //  TRC_FUNCTION_ENTER(PAR(iface));
  //  m_serializer = iface;
  //  TRC_FUNCTION_LEAVE("")
  //}

  //void LegacyApiSupport::detachInterface(iqrf::IJsonSerializerService* iface)
  //{
  //  TRC_FUNCTION_ENTER(PAR(iface));
  //  if (m_serializer == iface) {
  //    m_serializer = nullptr;
  //  }
  //  TRC_FUNCTION_LEAVE("")
  //}

  void LegacyApiSupport::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_dpa = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void LegacyApiSupport::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_dpa == iface) {
      m_dpa = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void LegacyApiSupport::attachInterface(iqrf::ISchedulerService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_scheduler = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void LegacyApiSupport::detachInterface(iqrf::ISchedulerService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_scheduler == iface) {
      m_scheduler = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void LegacyApiSupport::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void LegacyApiSupport::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
