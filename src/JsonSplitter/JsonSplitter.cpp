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

#define IMessagingSplitterService_EXPORTS

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

#include "TaskQueue.h"
#include "ApiMsg.h"
#include "JsonSplitter.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/pointer.h"
#include "rapidjson/schema.h"
#include "Trace.h"

#ifdef SHAPE_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

#include <mutex>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <locale>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__JsonSplitter.hxx"

TRC_INIT_MODULE(iqrf::JsonSplitter)

using namespace rapidjson;

static std::string m_schemesDir;

static const rapidjson::Document * fetchDocument(const std::string &uri) {
  rapidjson::Document *fetchedRoot = new rapidjson::Document();
  if (!valijson::utils::loadDocument(m_schemesDir + '/' + uri, *fetchedRoot)) {
    return nullptr;
  }
  return fetchedRoot;
}

static void freeDocument(const rapidjson::Document *adapter) {
  delete adapter;
}

namespace iqrf {

  class JsonSplitter::Imp {
  private:
    /// Messaging ID and message pair
    typedef std::pair<MessagingInstance, std::vector<uint8_t>> MsgIdMsg;
    /// Instance ID
    std::string m_insId = "iqrfgd2-default";
    /// Validate responses
    bool m_validateResponse = true;
    /// List of messaging services
    std::list<MessagingInstance> m_messagingList;
    /// Directory containing request and response schemas
    //std::string m_schemesDir;
    /// Messaging service mutex
    mutable std::mutex m_iMessagingServiceMapMux;
    /// Messaging service map
    std::map<MessagingInstance, IMessagingService*> m_iMessagingServiceMap;
    /// Message handling mutex
    mutable std::mutex m_filterMessageHandlerFuncMapMux;
    /// Registered message handlers
    std::map<std::string, FilteredMessageHandlerFunc > m_filterMessageHandlerFuncMap;
    /// Map of requests and validation schemas
    std::map<std::string, rapidjson::Document> m_validatorMapRequest;
    /// Map of responses and validation schemas
    std::map<std::string, rapidjson::Document> m_validatorMapResponse;
    /// Message type to handle
    std::map<std::string, MsgType> m_msgTypeToHandle;
    /// Management queue capacity
    size_t m_managementQueueCapacity = 0;
    /// Management message queue
    TaskQueue<MsgIdMsg>* m_managementQueue = nullptr;
    /// Network queue capacity
    size_t m_networkQueueCapacity = 0;
    /// Network message queue
    TaskQueue<MsgIdMsg>* m_networkQueue = nullptr;
    /// Launch service interface
    shape::ILaunchService* m_iLaunchService = nullptr;
    /// Management queue message whitelist
    const std::set<std::string> m_managementQueueWhitelist = {
      "iqrfDb_GetBinaryOutputs",
      "iqrfDb_GetDevice",
      "iqrfDb_GetDevices",
      "iqrfDb_GetDeviceMetadata",
      "iqrfDb_GetLights",
      "iqrfDb_GetNetworkTopology",
      "iqrfDb_GetSensors",
      "iqrfDb_Reset",
      "iqrfDb_SetDeviceMetadata",
      "mngDaemon_Exit",
      "mngDaemon_GetMode",
      "mngDaemon_SetMode",
      "mngDaemon_StartNetworkQueue",
      "mngDaemon_StopNetworkQueue",
      "mngDaemon_UpdateCache",
      "mngDaemon_Version",
      "mngScheduler_AddTask",
      "mngScheduler_EditTask",
      "mngScheduler_GetTask",
      "mngScheduler_List",
      "mngScheduler_RemoveAll",
      "mngScheduler_RemoveTask",
      "mngScheduler_StartTask",
      "mngScheduler_StopTask",
      "ntfDaemon_InvokeMonitor"
    };
  public:

    // for logging only
    static std::string JsonToStr(const rapidjson::Document& doc) {
      StringBuffer buffer;
      Writer<StringBuffer> writer(buffer);
      doc.Accept(writer);
      return buffer.GetString();
    }

    MsgType getMessageType(const rapidjson::Document& doc) const {
      std::string mType;
      std::string ver;

      //default version
      int major = 1;
      int minor = 0;
      int micro = 0;

      // get message type
      if (const Value* mTypeVal = rapidjson::Pointer("/mType").Get(doc)) {
        mType = mTypeVal->GetString();
      } else {
        THROW_EXC_TRC_WAR(std::logic_error, "Missing message type");
      }

      // get version
      if (const Value* verVal = rapidjson::Pointer("/ver").Get(doc)) {
        ver = verVal->GetString();
        std::replace(ver.begin(), ver.end(), '.', ' ');
        std::istringstream istr(ver);
        istr >> major >> minor >> micro;
      }

      std::string key = mType + '.' + std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(micro);

      auto handled = m_msgTypeToHandle.find(key);
      if (handled == m_msgTypeToHandle.end()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Unsupported message type: " << mType);
      }

      return handled->second;
    }

    void sendMessage(const MessagingInstance &messaging, rapidjson::Document doc) const {
      std::list<MessagingInstance> messagingList = { messaging };
      sendMessage(messagingList, std::move(doc));
    }

    void sendMessage(const std::list<MessagingInstance> &messagingList, rapidjson::Document doc) const {
      using namespace rapidjson;

      // Include instance ID in messages
      Pointer("/data/insId").Set(doc, m_insId);

      TRC_INFORMATION("Outgoing message: " << std::endl << JsonToStr(doc));

      // Check if message is allowed or supported
      MsgType mType = getMessageType(doc);

      // Validate generated response
      if (m_validateResponse) {
        validate(mType, doc, m_validatorMapResponse, "response");
      }

      StringBuffer buffer;
      PrettyWriter<StringBuffer> writer(buffer);
      doc.Accept(writer);

      // Send responses out
      if (messagingList.empty() && m_messagingList.empty()) {
        // Service and splitter messaging lists empty, send to all
        TRC_INFORMATION("No service or splitter messagings specified, sending to all available.");
        std::unique_lock<std::mutex> lock(m_iMessagingServiceMapMux);
        for (auto [instance, service] : m_iMessagingServiceMap) {
          if (service->acceptAsyncMsg()) {
            service->sendMessage(instance, std::basic_string<uint8_t>((uint8_t *)buffer.GetString(), buffer.GetSize()));
            TRC_INFORMATION("Outgoing message successfully sent.");
          }
        }
      } else {
        // Service or splitter messaging list not empty
        std::list<MessagingInstance> messagings = messagingList.empty() ? m_messagingList : messagingList;
        // Filter duplicates
        messagings.sort();
        messagings.unique();

        // Log outgoing message
        std::ostringstream oss;
        oss << "Messaging IDs: [";
        std::list<MessagingInstance>::iterator itr;
        for (itr = messagings.begin(); itr != messagings.end(); ++itr) {
          oss << itr->to_string();
          if (std::distance(itr, messagings.end()) > 1) {
            oss << ", ";
          }
        }
        oss << "]";
        TRC_INFORMATION(oss.str());

        // Send message to all specified messagings
        for (auto messaging : messagings) {
          std::string auxInstance(messaging.instance);
          // Check for appended topics
          size_t pos = auxInstance.find_first_of('/');
          if (pos != std::string::npos) {
            auxInstance = auxInstance.substr(0, pos);
          }

          auto auxMessaging = messaging;
          auxMessaging.instance = auxInstance;

          std::unique_lock<std::mutex> lock(m_iMessagingServiceMapMux);
          auto messsagingResult = m_iMessagingServiceMap.find(auxMessaging);
          if (messsagingResult != m_iMessagingServiceMap.end()) {
            messsagingResult->second->sendMessage(messaging, std::basic_string<uint8_t>((uint8_t *)buffer.GetString(), buffer.GetSize()));
            TRC_INFORMATION("Outgoing message sent via: " << messaging.to_string());
          } else {
            TRC_WARNING("Could not find required messaging: " << messaging.to_string());
          }
        }
      }
    }

    void registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, FilteredMessageHandlerFunc handlerFunc) {
      std::lock_guard<std::mutex> lck(m_filterMessageHandlerFuncMapMux);
      for (const auto & ft : msgTypeFilters) {
        m_filterMessageHandlerFuncMap.insert(std::make_pair(ft, handlerFunc));
      }
    }

    void unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters) {
      std::lock_guard<std::mutex> lck(m_filterMessageHandlerFuncMapMux);
      for (const auto & ft : msgTypeFilters) {
        m_filterMessageHandlerFuncMap.erase(ft);
      }
    }

    void validate(const IMessagingSplitterService::MsgType & msgType, const Document& doc,
      const std::map<std::string, rapidjson::Document>& validators, const std::string& direction) const {
      TRC_FUNCTION_ENTER(PAR(msgType.m_type));
      auto found = validators.find(msgType.getKey());
      if (found != validators.end()) {
        // parse schema
        valijson::Schema schema;
        valijson::SchemaParser parser;
        valijson::adapters::RapidJsonAdapter schemaAdapter(found->second);
        try {
          parser.populateSchema(schemaAdapter, schema, fetchDocument, freeDocument);
        } catch (std::exception &e) {
          THROW_EXC_TRC_WAR(std::logic_error, "Failed to parse jsonschema: " << e.what());
        }

        valijson::Validator validator(valijson::Validator::kStrongTypes);
        valijson::ValidationResults errors;
        valijson::adapters::RapidJsonAdapter adapter(doc);

        if (!validator.validate(schema, adapter, &errors)) {
          valijson::ValidationResults::Error error;
          while (errors.popError(error)) {
            std::string context;
            std::vector<std::string>::iterator itr = error.context.begin();
            for (; itr != error.context.end(); ++itr) {
              context += *itr;
            }
            THROW_EXC_TRC_WAR(std::logic_error, "Failed to validate " << direction << " message. Violating member: "
              << context << ". Violation: " << error.description
            );
          }
        }
        TRC_INFORMATION("Message successfully validated.")
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid " << direction << ": " <<
          NAME_PAR(mType, msgType.m_type) << " cannot find validator");
      }
      TRC_FUNCTION_LEAVE("")
    }

    void handleMessageFromMessaging(const MessagingInstance &messaging, const std::vector<uint8_t>& message) const {
      TRC_FUNCTION_ENTER(PAR(messaging.instance));
      using namespace rapidjson;

      std::string msgStr((char*)message.data(), message.size());

      TRC_INFORMATION("Incoming message:\n"
        << NAME_PAR(Messaging ID, messaging.instance)
        << "\n"
        << NAME_PAR(Message, msgStr)
      );

      std::string msgId("unknown");

      try {
        Document doc;
        doc.Parse(msgStr.c_str());

        /// Check for JSON syntax errors
        if (doc.HasParseError()) {
          TRC_WARNING("Failed to parse JSON message: error " << doc.GetParseError() << " at position " << doc.GetErrorOffset());
          sendMessage(messaging, JsonParseErrorMsg::createMessage(msgStr, doc.GetParseError(), doc.GetErrorOffset()));
          return;
        }

        msgId = Pointer("/data/msgId").GetWithDefault(doc, "unknown").GetString();

        /// Check for missing mType
        if (!doc.HasMember("mType")) {
          TRC_WARNING("mType missing in JSON message: " << msgStr);
          sendMessage(messaging, MissingMTypeMsg::createMessage(msgId, msgStr));
          return;
        }

        /// Validate request message
        MsgType msgType = getMessageType(doc);
        try {
          validate(msgType, doc, m_validatorMapRequest, "request");
        } catch (const std::logic_error &e) {
          TRC_WARNING("Failed to validate JSON request: " << e.what());
          sendMessage(messaging, ValidationErrorMsg::createMessage(msgId, msgStr, e.what()));
          return;
        }

        if (m_managementQueueWhitelist.find(msgType.m_type) != m_managementQueueWhitelist.end()) {
          handleManagementMessageFromMessaging(messaging, message, msgType.m_type, msgId);
        } else {
          handleNetworkMessageFromMessaging(messaging, message, msgType.m_type, msgId);
        }
      } catch (const std::exception &e) {
        TRC_WARNING("Failed to process request from messaging: " << e.what());
        try {
          sendMessage(messaging, GeneralErrorMsg::createMessage(msgId, msgStr, e.what()));
        } catch (const std::exception &ee) {
          TRC_WARNING("Failed to send general error response: " << ee.what());
        }
      }
    }

    void handleManagementMessageFromMessaging(const MessagingInstance &messaging, const std::vector<uint8_t> &message, const std::string &mType, const std::string &msgId) const {
      if (!m_managementQueue) {
        TRC_WARNING("Management message queue has not been initialized.");
        sendMessage(messaging, MessageQueueNotInitializedErrorMsg::createMessage(msgId, mType, false));
        return;
      }
      auto queueLen = m_managementQueue->size();
      if (queueLen < m_managementQueueCapacity) {
        m_managementQueue->pushToQueue(std::make_pair(messaging, message));
      } else {
        TRC_WARNING("Management queue full, message " << mType << ":" << msgId << " discarded.");
        sendMessage(messaging, MessageQueueFullErrorMsg::createMessage(msgId, mType, false, m_managementQueueCapacity));
      }
      TRC_FUNCTION_LEAVE(PAR(queueLen))
    }

    void handleNetworkMessageFromMessaging(const MessagingInstance &messaging, const std::vector<uint8_t> &message, const std::string &mType, const std::string &msgId) const {
      if (!m_networkQueue) {
        TRC_WARNING("Network message queue has not been initialized.");
        sendMessage(messaging, MessageQueueNotInitializedErrorMsg::createMessage(msgId, mType, true));
        return;
      }
      auto queueLen = m_networkQueue->size();
      if (queueLen < m_networkQueueCapacity) {
        m_networkQueue->pushToQueue(std::make_pair(messaging, message));
      } else {
        TRC_WARNING("Network queue full, message " << mType << ":" << msgId << " discarded.");
        sendMessage(messaging, MessageQueueFullErrorMsg::createMessage(msgId, mType, true, m_networkQueueCapacity));
      }
      TRC_FUNCTION_LEAVE(PAR(queueLen))
    }

    int getManagementQueueLen() const {
      if (m_managementQueue) {
        return static_cast<int>(m_managementQueue->size());
      }
      return -1;
    }

    int getNetworkQueueLen() const {
      if (m_networkQueue) {
        return static_cast<int>(m_networkQueue->size());
      }
      return -1;
    }

    void handleNetworkQueueMessages(const MessagingInstance &messaging, MsgType msgType, rapidjson::Document rq) {
      try {
        rapidjson::Document rsp;
        rapidjson::Pointer("/mType").Set(rsp, msgType.m_type);
        rapidjson::Pointer("/data/msgId").Set(rsp, rapidjson::Pointer("/data/msgId").GetWithDefault(rq, "unknown"));
        if (msgType.m_type == "mngDaemon_StartNetworkQueue" && !m_networkQueue->isActive()) {
          m_networkQueue->startQueue();
        } else if (msgType.m_type == "mngDaemon_StopNetworkQueue" && m_networkQueue->isActive()) {
          m_networkQueue->stopQueue();
        }
        rapidjson::Pointer("/data/status").Set(rsp, 0);
        rapidjson::Pointer("/data/statusStr").Set(rsp, "ok");
        sendMessage(messaging, std::move(rsp));
      } catch (const std::logic_error &e) {
        THROW_EXC_TRC_WAR(std::logic_error, e.what());
      }
    }

    void handleMessageFromSplitterQueue(const MessagingInstance &messaging, const std::vector<uint8_t>& message) const {
      using namespace rapidjson;

      std::string str((char*)message.data(), message.size());
      StringStream sstr(str.data());
      Document doc;
      doc.ParseStream(sstr);
      std::string msgId("undefined");

      try {
        msgId = Pointer("/data/msgId").GetWithDefault(doc, "undefined").GetString();
        MsgType msgType = getMessageType(doc);

        std::map<std::string, FilteredMessageHandlerFunc > bestFitMap;
        { //lock scope
          if (msgType.m_type == "mngDaemon_Exit") {
            m_networkQueue->stopQueue();
          }
          //std::lock_guard<std::mutex> lck(m_filterMessageHandlerFuncMapMux);
          for (const auto & filter : m_filterMessageHandlerFuncMap) {
            // best fit
            if (std::string::npos != msgType.m_type.find(filter.first)) {
              bestFitMap.insert(filter);
            }
          }
          if (!bestFitMap.empty()) {
            size_t len = 0;
            FilteredMessageHandlerFunc selected;
            for (const auto & fit : bestFitMap) {
              if (len < fit.first.size()) {
                selected = fit.second;
                len = fit.first.size();
              }
            }
            // invoke handling
            try {
              selected(messaging, msgType, std::move(doc));
              TRC_INFORMATION("Incoming message successfully handled.");
            } catch (std::exception &e) {
              THROW_EXC_TRC_WAR(std::logic_error, "Unhandled exception: " << e.what());
            }
          } else {
            THROW_EXC_TRC_WAR(std::logic_error, "Unsupported: " << NAME_PAR(mType.version, msgType.getKey()));
          }
        }
      } catch (std::logic_error &e) {
        TRC_WARNING("Error while handling incoming message:" << e.what());
        try {
          sendMessage(messaging, GeneralErrorMsg::createMessage(msgId, str, e.what()));
        } catch (const std::logic_error &ee) {
          TRC_WARNING("Cannot create error response:" << ee.what());
        }
      }
    }

#ifdef SHAPE_PLATFORM_WINDOWS
    std::vector<std::string> getSchemesFiles(const std::string& schemesDir, const std::string& filter)
    {
      WIN32_FIND_DATA fid;
      HANDLE found = INVALID_HANDLE_VALUE;

      std::vector<std::string> fileVect;
      std::string sdirect(schemesDir);
      sdirect.append("/*");
      sdirect.append(filter);
      sdirect.append("*");

      found = FindFirstFile(sdirect.c_str(), &fid);

      if (INVALID_HANDLE_VALUE == found) {
        THROW_EXC_TRC_WAR(std::logic_error, "JsonSchemes directory does not exist: " << PAR(schemesDir));
      }

      do {
        if (fid.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          continue; //skip a directory
        std::string fil(schemesDir);
        fil.append("/");
        fil.append(fid.cFileName);
        fileVect.push_back(fil);
      } while (FindNextFile(found, &fid) != 0);

      FindClose(found);
      return fileVect;
    }

#else
    std::vector<std::string> getSchemesFiles(const std::string& schemesDir, const std::string& filter) {
      std::vector<std::string> fileVect;

      DIR *dir;
      class dirent *ent;
      class stat st;

      dir = opendir(schemesDir.c_str());
      if (dir == nullptr) {
        THROW_EXC_TRC_WAR(std::logic_error, "JsonSchemes directory does not exist: " << PAR(schemesDir));
      }
      //TODO exeption if dir doesn't exists
      while ((ent = readdir(dir)) != NULL) {
        const std::string file_name = ent->d_name;
        const std::string full_file_name(schemesDir + "/" + file_name);

        if (file_name[0] == '.')
          continue;

        if (std::string::npos == file_name.find(filter))
          continue;

        if (stat(full_file_name.c_str(), &st) == -1)
          continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;

        if (is_directory)
          continue;

        fileVect.push_back(full_file_name);
      }
      closedir(dir);


      return fileVect;
    }

#endif

    void loadJsonSchemesRequest(const std::string sdir) {
      TRC_FUNCTION_ENTER(PAR(sdir));

      //std::vector<std::string> files = getSchemesFiles(sdir, "-request-");
      std::vector<std::string> files = getSchemesFiles(sdir, ".json");

      for (const auto & fname : files) {
        TRC_INFORMATION("loading: " << PAR(fname));
        try {
          Document sd;

          std::ifstream ifs(fname);
          if (!ifs.is_open()) {
            THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(fname));
          }

          rapidjson::IStreamWrapper isw(ifs);
          sd.ParseStream(isw);

          if (sd.HasParseError()) {
            THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " << NAME_PAR(emsg, sd.GetParseError()) <<
              NAME_PAR(eoffset, sd.GetErrorOffset()));
          }

          // get version and device name from file name
          std::string fname2 = fname;
          std::size_t found = fname2.find_last_of("/\\");
          if (found != std::string::npos) {
            fname2 = fname2.substr(found + 1);
          }

          std::replace(fname2.begin(), fname2.end(), '-', '.');
          std::replace(fname2.begin(), fname2.end(), '.', ' ');
          std::string direction, fileN, possibleDriverFunction;
          int major = 1;
          int minor = 0;
          int micro = 0;
          std::istringstream os(fname2);
          os >> fileN >> direction >> major >> minor >> micro;
          std::replace(fileN.begin(), fileN.end(), '/', ' ');
          std::istringstream os1(fileN);
          while (!os1.eof()) {
            os1 >> fileN;
          }
          bool stop_ = false;
          std::locale loc;
          for (auto ch : fileN) {
            if (!stop_ && std::isupper(ch, loc)) {
              possibleDriverFunction.push_back('.');
              possibleDriverFunction.push_back(std::tolower(ch, loc));
            }
            else {
              possibleDriverFunction.push_back(ch);
            }
            if (ch == '_') {
              stop_ = true;
            }
          }
          std::replace(possibleDriverFunction.begin(), possibleDriverFunction.end(), '_', '.');

          // preparse key
          std::string mType;
          std::string ver;

          // get message type
          Value* mTypeVal = nullptr;

          Value* allOf = Pointer("/properties/mType/allOf").Get(sd);
          if (allOf != nullptr && allOf->IsArray() && allOf->Size() > 0) {
            for (auto &item : allOf->GetArray()) {
              auto found = item.FindMember("const");
              if (found != item.MemberEnd()) {
                mTypeVal = &found->value;
                break;
              }
            }
          }
          if (mTypeVal == nullptr) {
            mTypeVal = Pointer("/properties/mType/const").Get(sd);
          }
          if (mTypeVal == nullptr) {
            mTypeVal = Pointer("/properties/mType/enum/0").Get(sd);
          }
          if (mTypeVal != nullptr) {
            mType = mTypeVal->GetString();
          } else {
            //defaulted to support daemon V1 messages
            THROW_EXC_TRC_WAR(std::logic_error, "Invalid schema: " << PAR(fname));
          }

          MsgType msgType(mType, major, minor, micro, possibleDriverFunction);
          auto key = msgType.getKey();

          if (direction == "request") {
            m_validatorMapRequest.insert(std::make_pair(key, std::move(sd)));
          } else if (direction == "response") {
            m_validatorMapResponse.insert(std::make_pair(key, std::move(sd)));
          }
          m_msgTypeToHandle.insert(std::make_pair(key, msgType));
          TRC_DEBUG("Added: "
            << PAR(key)
            << PAR(msgType.m_type)
            << " ver" << msgType.m_major << '.' << msgType.m_minor << '.' << msgType.m_micro
            << " drv:" << msgType.m_possibleDriverFunction
          );
        } catch (std::exception & e) {
          CATCH_EXC_TRC_WAR(std::exception, e, "");
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonSplitter instance activate" << std::endl <<
        "******************************"
      );

      modify(props);

      m_schemesDir = m_iLaunchService->getDataDir() + "/apiSchemas";
      TRC_INFORMATION("loading schemes from: " << PAR(m_schemesDir));
      loadJsonSchemesRequest(m_schemesDir);

      m_managementQueue = shape_new TaskQueue<MsgIdMsg>([&](const MsgIdMsg &msgIdMsg) {
        handleMessageFromSplitterQueue(msgIdMsg.first, msgIdMsg.second);
      });
      m_networkQueue = shape_new TaskQueue<MsgIdMsg>([&](const MsgIdMsg& msgIdMsg) {
        handleMessageFromSplitterQueue(msgIdMsg.first, msgIdMsg.second);
      });

      registerFilteredMsgHandler(
        {
          "mngDaemon_StartNetworkQueue",
          "mngDaemon_StopNetworkQueue"
        },
        [&](const MessagingInstance &messaging, const MsgType &msgType, rapidjson::Document doc) {
          handleNetworkQueueMessages(messaging, msgType, std::move(doc));
        }
      );

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props) {
      props->getMemberAsString("insId", m_insId);
      props->getMemberAsBool("validateJsonResponse", m_validateResponse);
      m_messagingList.clear();

      const Document &doc = props->getAsJson();
      /// Messaging list
      const Value *val = Pointer("/messagingList").Get(doc);
      if (val && val->IsArray()) {
        const auto arr = val->GetArray();
        for (auto itr = arr.Begin(); itr != arr.End(); ++itr) {
          auto type = Pointer("/type").Get(*itr)->GetString();
          auto instance = Pointer("/instance").Get(*itr)->GetString();
          m_messagingList.push_back(
            MessagingInstance(
              type,
              instance
            )
          );
        }
        m_messagingList.sort();
        m_messagingList.unique();
      }
      // Management queue capacity
      val = Pointer("/managementQueueCapacity").Get(doc);
      if (val && val->IsUint64()) {
        m_managementQueueCapacity = val->GetUint64();
      }
      // Network queue capacity
      val = Pointer("/networkQueueCapacity").Get(doc);
      if (val && val->IsUint64()) {
        m_networkQueueCapacity = val->GetUint64();
      }
      TRC_INFORMATION(PAR(m_validateResponse));
    }

    void deactivate() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonSplitter instance deactivate" << std::endl <<
        "******************************"
      );

      delete m_networkQueue;
      delete m_managementQueue;

      TRC_FUNCTION_LEAVE("")
    }

    void attachInterface(shape::ILaunchService* iface) {
      m_iLaunchService = iface;
    }

    void detachInterface(shape::ILaunchService* iface) {
      if (m_iLaunchService == iface) {
        m_iLaunchService = nullptr;
      }
    }

    void attachInterface(iqrf::IMessagingService* iface) {
      //TODO shall be targeted only to JSON content or "iqrf-daemon-api"
      std::unique_lock<std::mutex> lck(m_iMessagingServiceMapMux);
      auto candidate = iface->getMessagingInstance();
      if (m_iMessagingServiceMap.find(candidate) != m_iMessagingServiceMap.end()) {
        TRC_WARNING("Messaging instance " + candidate.instance + " already exists.");
        return;
      }
      m_iMessagingServiceMap.insert(std::make_pair(candidate, iface));
      iface->registerMessageHandler([&](const MessagingInstance &messaging, const std::vector<uint8_t>& message)
      {
        handleMessageFromMessaging(messaging, message);
      });
    }

    void detachInterface(iqrf::IMessagingService* iface) {
      std::unique_lock<std::mutex> lck(m_iMessagingServiceMapMux);
      {
        auto candidate = iface->getMessagingInstance();
        auto found = m_iMessagingServiceMap.find(candidate);
        if (found != m_iMessagingServiceMap.end() && found->second == iface) {
          iface->unregisterMessageHandler();
          m_iMessagingServiceMap.erase(found);
        }
      }
    }

    std::string getSchemasDir() {
      return m_schemesDir;
    }

  };

  ////////////////////////////////
  JsonSplitter::JsonSplitter()
  {
    m_imp = shape_new Imp();
  }

  JsonSplitter::~JsonSplitter()
  {
    delete m_imp;
  }

  void JsonSplitter::sendMessage(const MessagingInstance &messaging, rapidjson::Document doc) const
  {
    m_imp->sendMessage(messaging, std::move(doc));
  }

  void JsonSplitter::sendMessage(const std::list<MessagingInstance> &messagings, rapidjson::Document doc) const {
    m_imp->sendMessage(messagings, std::move(doc));
  }

  void JsonSplitter::registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, FilteredMessageHandlerFunc handlerFunc)
  {
    m_imp->registerFilteredMsgHandler(msgTypeFilters, handlerFunc);
  }

  void JsonSplitter::unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters)
  {
    m_imp->unregisterFilteredMsgHandler(msgTypeFilters);
  }

  int JsonSplitter::getManagementQueueLen() const {
   return m_imp->getManagementQueueLen();
  }

  int JsonSplitter::getNetworkQueueLen() const {
    return m_imp->getNetworkQueueLen();
  }

  void JsonSplitter::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonSplitter::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonSplitter::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonSplitter::attachInterface(shape::ILaunchService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonSplitter::detachInterface(shape::ILaunchService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonSplitter::attachInterface(iqrf::IMessagingService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonSplitter::detachInterface(iqrf::IMessagingService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonSplitter::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonSplitter::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
