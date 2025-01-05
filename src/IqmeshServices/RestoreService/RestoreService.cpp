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

#define IRestoreService_EXPORTS

#include "RestoreService.h"
#include "Trace.h"
#include "ComRestore.h"
#include "iqrf__RestoreService.hxx"
#include <list>
#include <cmath>
#include <thread>
#include <bitset>
#include <chrono>

TRC_INIT_MODULE(iqrf::RestoreService)

using namespace rapidjson;

namespace
{
  static const int serviceError = 1000;
  static const int parsingRequestError = 1001;
  static const int exclusiveAccessError = 1002;
}

namespace iqrf {
  // Implementation class
  class RestoreService::Imp
  {
  private:
    // Parent object
    RestoreService & m_parent;

    // Message type
    const std::string m_mTypeName_Restore = "iqmeshNetwork_Restore";
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfRestore* m_iIqrfRestore = nullptr;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComRestore* m_comRestore = nullptr;

  public:
    Imp(RestoreService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

    //--------------------
    // Send restore result
    //--------------------
    void sendRestoreResult(const int status, const std::string statusStr)
    {
      Document docRestoreResult;

      // Set common parameters
      Pointer("/mType").Set(docRestoreResult, m_msgType->m_type);
      Pointer("/data/msgId").Set(docRestoreResult, m_comRestore->getMsgId());

      // Set status
      Pointer("/data/status").Set(docRestoreResult, status);
      Pointer("/data/statusStr").Set(docRestoreResult, statusStr);

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(docRestoreResult));
    }

    //--------------------
    // Send restore result
    //--------------------
    void sendRestoreResult(const int status, const std::string statusStr, const TRestoreInputParams& backupData, const double progress)
    {
      Document docRestoreResult;

      // Set common parameters
      Pointer("/mType").Set(docRestoreResult, m_msgType->m_type);
      Pointer("/data/msgId").Set(docRestoreResult, m_comRestore->getMsgId());

      // Add progress
      int p = (int)round(progress);
      Pointer("/data/rsp/progress").Set(docRestoreResult, p);

      // Add backup result
      rapidjson::Value arrayDevices(kArrayType);
      Document::AllocatorType& allocator = docRestoreResult.GetAllocator();
      rapidjson::Value objDeviceBackup(kObjectType);
      // Put device address
      objDeviceBackup.AddMember("deviceAddr", backupData.deviceAddress, allocator);
      // Put restoreStatus
      objDeviceBackup.AddMember("restoreStatus", statusStr == "ok", allocator);
      // Put device
      arrayDevices.PushBack(objDeviceBackup, allocator);
      Pointer("/data/rsp/devices").Set(docRestoreResult, arrayDevices);

      // Set raw fields, if verbose mode is active
      if (m_comRestore->getVerbose() == true)
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = docRestoreResult.GetAllocator();
        std::list<std::unique_ptr<IDpaTransactionResult2>> transResults;
        m_iIqrfRestore->getTransResults(transResults);
        while(transResults.size() != 0)
        {
          std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = transResults.begin();
          std::unique_ptr<IDpaTransactionResult2> tr = std::move(*iter);
          transResults.pop_front();

          rapidjson::Value rawObject(kObjectType);

          rawObject.AddMember(
            "request",
            encodeBinary(tr->getRequest().DpaPacket().Buffer, tr->getRequest().GetLength()),
            allocator
          );

          rawObject.AddMember(
            "requestTs",
            TimeConversion::encodeTimestamp(tr->getRequestTs()),
            allocator
          );

          rawObject.AddMember(
            "confirmation",
            encodeBinary(tr->getConfirmation().DpaPacket().Buffer, tr->getConfirmation().GetLength()),
            allocator
          );

          rawObject.AddMember(
            "confirmationTs",
            TimeConversion::encodeTimestamp(tr->getConfirmationTs()),
            allocator
          );

          rawObject.AddMember(
            "response",
            encodeBinary(tr->getResponse().DpaPacket().Buffer, tr->getResponse().GetLength()),
            allocator
          );

          rawObject.AddMember(
            "responseTs",
            TimeConversion::encodeTimestamp(tr->getResponseTs()),
            allocator
          );

          // add object into array
          rawArray.PushBack(rawObject, allocator);
        }

        // Add array into response document
        Pointer("/data/raw").Set(docRestoreResult, rawArray);
      }

      // Set status
      Pointer("/data/status").Set(docRestoreResult, status);
      Pointer("/data/statusStr").Set(docRestoreResult, statusStr);

      // Send message
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(docRestoreResult));
    }

    //------------
    // Run restore
    //------------
    void runRestore(TRestoreInputParams& backupData)
    {
      TRC_FUNCTION_ENTER("");
      // Restore selected devices
      std::string statusStr = "ok";
      try
      {
        // Parse backup data string
        std::basic_string<uint8_t> data;
        data.clear();
        for (size_t i = 0x00; i < backupData.data.size(); i += 0x02)
        {
          std::string byteString = backupData.data.substr(i, 2);
          uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
          data.push_back(byte);
        }
        // Restore device
        m_iIqrfRestore->restore(backupData.deviceAddress, data, backupData.restartCoodinator);
      }
      catch (std::exception& e)
      {
        statusStr = e.what();
        CATCH_EXC_TRC_WAR(std::exception, e, "Restore device [" << (int)backupData.deviceAddress << "] error.");
      }
      sendRestoreResult(m_iIqrfRestore->getErrorCode(), statusStr, backupData, 100.0);
      TRC_FUNCTION_LEAVE("");
    }

    // Process request
    void handleMsg(const std::string& messagingId, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) << NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      // Unsupported type of request
      if (msgType.m_type != m_mTypeName_Restore)
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));

      // Create representation object
      ComRestore comRestore(doc);
      m_msgType = &msgType;
      m_messagingId = &messagingId;
      m_comRestore = &comRestore;

      // Parsing and checking service parameters
      TRestoreInputParams restoreInputParams;
      try
      {
        restoreInputParams = comRestore.getRestoreParams();
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Error while parsing service input parameters.")
        sendRestoreResult(parsingRequestError, e.what());
        TRC_FUNCTION_LEAVE("");
        return;
      }

      // Run the Restore
      try
      {
        runRestore(restoreInputParams);
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, e.what());
        sendRestoreResult(m_iIqrfRestore->getErrorCode(), e.what());
      }
      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "Backup instance activate" << std::endl <<
        "************************************"
      );

      modify(props);

      // for the sake of register function parameters
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_Restore
      };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
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
        "************************************" << std::endl <<
        "Bqackup instance deactivate" << std::endl <<
        "************************************"
      );

      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_Restore
      };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(supportedMsgTypes);

      TRC_FUNCTION_LEAVE("");
    }

    void modify(const shape::Properties *props)
    {
      (void)props;
    }

    void attachInterface(IIqrfRestore* iface)
    {
      m_iIqrfRestore = iface;
    }

    void detachInterface(IIqrfRestore* iface)
    {
      if (m_iIqrfRestore == iface)
        m_iIqrfRestore = nullptr;
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

  RestoreService::RestoreService()
  {
    m_imp = shape_new Imp(*this);
  }

  RestoreService::~RestoreService()
  {
    delete m_imp;
  }

  void RestoreService::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void RestoreService::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void RestoreService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void RestoreService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void RestoreService::attachInterface(IIqrfRestore* iface)
  {
    m_imp->attachInterface(iface);
  }

  void RestoreService::detachInterface(IIqrfRestore* iface)
  {
    m_imp->detachInterface(iface);
  }

  void RestoreService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void RestoreService::deactivate()
  {
    m_imp->deactivate();
  }

  void RestoreService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}

