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

#define IMessagingSplitterService_EXPORTS

#include "TaskQueue.h"
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
    std::string m_schemesDir;
    /// Messaging service mutex
    mutable std::mutex m_iMessagingServiceMapMux;
    /// Messaging service map
    std::map<MessagingInstance, IMessagingService*> m_iMessagingServiceMap;
    /// Message handling mutex
    mutable std::mutex m_filterMessageHandlerFuncMapMux;
    /// Registered message handlers
    std::map<std::string, FilteredMessageHandlerFunc > m_filterMessageHandlerFuncMap;
    /// Map of requests and validation schemas
    std::map<std::string, rapidjson::SchemaDocument> m_validatorMapRequest;
    /// Map of responses and validation schemas
    std::map<std::string, rapidjson::SchemaDocument> m_validatorMapResponse;
    /// Message type to handle
    std::map<std::string, MsgType> m_msgTypeToHandle;
    /// Daemon network queue size
    std::size_t m_networkQueueSize = 32;
    /// Daemon network queue
    TaskQueue<MsgIdMsg>* m_networkQueue = nullptr;
    /// Daemon management queue size
    std::size_t m_managementQueueSize = 32;
    /// Daemon management queue
    TaskQueue<MsgIdMsg>* m_managementQueue = nullptr;
    /// Network queue control message filters
    std::vector<std::string> m_queueControlFilters = {
      "mngDaemon_StartQueue",
      "mngDaemon_StopQueue"
    };
    // Set of message types handled by management queue
    std::set<std::string> m_managementQueueMessageFilter = {
      "mngDaemon_StartQueue",
      "mngDaemon_StopQueue"
    };
    /// Launch service interface
    shape::ILaunchService* m_iLaunchService = nullptr;

    std::string getKey(const MsgType& msgType) const
    {
      std::ostringstream os;
      os << msgType.m_type << '.' << msgType.m_major << '.' << msgType.m_minor << '.' << msgType.m_patch;
      return os.str();
    }

  public:

    // for logging only
    static std::string JsonToStr(const rapidjson::Document& doc)
    {
      StringBuffer buffer;
      Writer<StringBuffer> writer(buffer);
      doc.Accept(writer);
      return buffer.GetString();
    }

    MsgType getMessageType(const rapidjson::Document& doc) const
    {
      using namespace rapidjson;
      std::string mType;
      std::string ver;

      //default version
      int major = 1;
      int minor = 0;
      int patch = 0;

      // get message type
      if (const Value* mTypeVal = Pointer("/mType").Get(doc)) {
        mType = mTypeVal->GetString();
      } else {
        THROW_EXC_TRC_WAR(std::invalid_argument, "Missing message type");
      }

      // get version
      if (const Value* verVal = Pointer("/ver").Get(doc)) {
        ver = verVal->GetString();
        std::replace(ver.begin(), ver.end(), '.', ' ');
        std::istringstream istr(ver);
        istr >> major >> minor >> patch;
      }

      std::ostringstream oss;
      oss << mType << '.' << major << '.' << minor << '.' << patch;
      std::string verKey = oss.str();

      // check if message type is supported
      auto match = m_msgTypeToHandle.find(verKey);
      if (match == m_msgTypeToHandle.end()) {
        THROW_EXC_TRC_WAR(std::domain_error, "Unsupported: " << NAME_PAR(mType, mType) << NAME_PAR(key, verKey));
      }

      return match->second;
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
      MsgType msgType = getMessageType(doc);

      // Validate generated response
      if (m_validateResponse) {
        validate(msgType, doc, m_validatorMapResponse, "response");
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

    void registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, FilteredMessageHandlerFunc handlerFunc)
    {
      std::lock_guard<std::mutex> lck(m_filterMessageHandlerFuncMapMux);
      for (const auto & ft : msgTypeFilters) {
        m_filterMessageHandlerFuncMap.insert(std::make_pair(ft, handlerFunc));
      }
    }

    void unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters)
    {
      std::lock_guard<std::mutex> lck(m_filterMessageHandlerFuncMapMux);
      for (const auto & ft : msgTypeFilters) {
        m_filterMessageHandlerFuncMap.erase(ft);
      }
    }

    void validate(const IMessagingSplitterService::MsgType & msgType, const Document& doc,
      const std::map<std::string, rapidjson::SchemaDocument>& validators, const std::string& direction) const
    {
      TRC_FUNCTION_ENTER(PAR(msgType.m_type))
      auto found = validators.find(getKey(msgType));
      if (found != validators.end()) {
        SchemaValidator validator(found->second);
        if (!doc.Accept(validator)) {
          // Input JSON is invalid according to the schema
          StringBuffer sb;
          std::string schema, keyword, document;
          validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
          schema = sb.GetString();
          keyword = validator.GetInvalidSchemaKeyword();
          sb.Clear();
          validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
          document = sb.GetString();
          THROW_EXC_TRC_WAR(std::logic_error, "Invalid " << direction << ": " <<
            NAME_PAR(mType, msgType.m_type) << PAR(schema) << PAR(keyword) << NAME_PAR(message, document));
        }
        //TRC_DEBUG("OK");
        TRC_INFORMATION("Message successfully validated.")
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid " << direction << ": " <<
          NAME_PAR(mType, msgType.m_type) << " cannot find validator");
      }
      TRC_FUNCTION_LEAVE("")
    }

    void sendErrorMessage(const MessagingInstance &messaging, rapidjson::Document doc) const {
      try {
        sendMessage(messaging, std::move(doc));
      } catch (const std::logic_error &e) {
        TRC_WARNING("Unable to send error response: " << e.what());
      }
    }
 
    void handleQueueManagementMsg(const MessagingInstance &messaging, const MsgType &msgType, rapidjson::Document doc) const {
      TRC_FUNCTION_ENTER(
				PAR(messaging.to_string()) <<
				NAME_PAR(mType, msgType.m_type) <<
				NAME_PAR(major, msgType.m_major) <<
				NAME_PAR(minor, msgType.m_minor) <<
				NAME_PAR(patch, msgType.m_patch)
			);
      
      if (msgType.m_type == "mngDaemon_StartQueue") {
        m_networkQueue->startQueue();
      } else if (msgType.m_type == "mngDaemon_StopQueue") {
        m_networkQueue->stopQueue();
      }
    }

    void handleMessageFromMessaging(const MessagingInstance &messaging, const std::vector<uint8_t>& message) const
    {
      TRC_FUNCTION_ENTER(PAR(messaging.instance));
      using namespace rapidjson;

      std::string msgStr((char*)message.data(), message.size());

      TRC_INFORMATION("Incoming message:\n"
        << NAME_PAR(Messaging ID, messaging.instance)
        << "\n"
        << NAME_PAR(Message, msgStr)
        );

      int queueLen = -1;

      StringStream ss(msgStr.data());
      Document doc;
      doc.ParseStream(ss);
      std::string msgId("unknown");

      try {
        if (doc.HasParseError()) {
          //TODO parse error handling => send back an error JSON with details
          THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " <<
            NAME_PAR(emsg, doc.GetParseError()) <<
            NAME_PAR(eoffset, doc.GetErrorOffset())
          );
        }
      } catch (const std::logic_error &e) {
        rapidjson::Document errDoc;
        JsonParseErrorMsg errMsg(msgStr);
        errMsg.createResponse(errDoc);
        sendErrorMessage(messaging, std::move(errDoc));
        return;
      }

      msgId = Pointer("/data/msgId").GetWithDefault(doc, "unknown").GetString();

      try {
        MsgType msgType = getMessageType(doc);

        const std::string REQS("request");
        validate(msgType, doc, m_validatorMapRequest, REQS);
      } catch (const std::invalid_argument &e) {

      } catch (const std::logic_error &e) {
        rapidjson::Document errDoc;
        InvalidMsg()
        return;
      }

      MsgType msgType = getMessageType(doc);
      if (m_managementQueueMessageFilter.count(msgType.m_type)) {
        // check availability of management queue
        if (m_managementQueue) {
          if (m_managementQueue->size() < m_managementQueueSize) {
            m_managementQueue->pushToQueue(std::make_pair(messaging, message));
          } else {
            TRC_WARNING("Management queue full, message ignored.");
            rapidjson::Document errDoc;
            QueueFullMsg errMsg(msgId, msgType.m_type, m_managementQueueSize, false);
            errMsg.createResponse(errDoc);
            sendErrorMessage(messaging, std::move(errDoc));
            return;
          }
        } else {
          TRC_WARNING("Management queue not available, message ignored.");
          rapidjson::Document errDoc;
          QueueInactiveMsg errMsg(msgId, msgType.m_type, false);
          errMsg.createResponse(errDoc);
          sendErrorMessage(messaging, std::move(errDoc));
          return;
        }
      } else {
        if (m_networkQueue) {
          if (m_networkQueue->size() < m_networkQueueSize) {
            m_networkQueue->pushToQueue(std::make_pair(messaging, message));
          } else {
            TRC_WARNING("Network queue full, message ignored.");
            rapidjson::Document errDoc;
            QueueFullMsg errMsg(msgId, msgType.m_type, m_managementQueueSize);
            errMsg.createResponse(errDoc);
            sendErrorMessage(messaging, std::move(errDoc));
            return;
          }
        } else {
          TRC_WARNING("Network queue not available, message ignored.");
          rapidjson::Document errDoc;
          QueueInactiveMsg errMsg(msgId, msgType.m_type);
          errMsg.createResponse(errDoc);
          sendErrorMessage(messaging, std::move(errDoc));
          return;
        }
      }
    }

    int getMsgQueueLen() const
    {
      return (int)m_networkQueue->size();
    }

    void handleMessageFromSplitterQueue(const MessagingInstance &messaging, const std::vector<uint8_t>& message) const
    {
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
          std::lock_guard<std::mutex> lck(m_filterMessageHandlerFuncMapMux);
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
            THROW_EXC_TRC_WAR(std::logic_error, "Unsupported: " << NAME_PAR(mType.version, getKey(msgType)));
          }
        }
      } catch (std::logic_error &e) {
        TRC_WARNING("Error while handling incoming message:" << e.what());

        Document rspDoc;
        MessageErrorMsg msg(msgId, str, e.what());
        msg.createResponse(rspDoc);
        try {
          sendMessage(messaging, std::move(rspDoc));
        }
        catch (std::logic_error &ee) {
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
    std::vector<std::string> getSchemesFiles(const std::string& schemesDir, const std::string& filter)
    {
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

    void loadJsonSchemesRequest(const std::string sdir)
    {
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
          if (Value* mTypeVal = Pointer("/properties/mType/enum/0").Get(sd)) {
            mType = mTypeVal->GetString();
          }
          else {
            //defaulted to support daemon V1 messages
            THROW_EXC_TRC_WAR(std::logic_error, "Invalid schema: " << PAR(fname));
          }

          MsgType msgType(mType, major, minor, micro, possibleDriverFunction);

          SchemaDocument schema(sd);

          if (direction == "request") {
            m_validatorMapRequest.insert(std::make_pair(getKey(msgType), std::move(schema)));
            //m_msgTypeToHandle.insert(std::make_pair(getKey(msgType), msgType));
          }
          else if (direction == "response") {
            m_validatorMapResponse.insert(std::make_pair(getKey(msgType), std::move(schema)));
          }
          m_msgTypeToHandle.insert(std::make_pair(getKey(msgType), msgType));
          TRC_DEBUG ("Added: " <<
            NAME_PAR(key, getKey(msgType)) <<
            PAR(msgType.m_type) <<
            NAME_PAR(ver, msgType.m_major << '.' << msgType.m_minor << '.' << msgType.m_patch) <<
            NAME_PAR(drv, msgType.m_possibleDriverFunction)
          );
        } catch (const std::exception &e) {
          CATCH_EXC_TRC_WAR(std::exception, e, "");
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
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

      m_managementQueue = new TaskQueue<MsgIdMsg>([&](const MsgIdMsg &msgIdMsg) {
        handleMessageFromSplitterQueue(msgIdMsg.first, msgIdMsg.second);
      });

      m_networkQueue = new TaskQueue<MsgIdMsg>([&](const MsgIdMsg& msgIdMsg) {
        handleMessageFromSplitterQueue(msgIdMsg.first, msgIdMsg.second);
      });

      registerFilteredMsgHandler(
        m_queueControlFilters,
        [&](const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc) {
          handleQueueManagementMsg(messaging, msgType, std::move(doc));
        }
      );

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      props->getMemberAsString("insId", m_insId);
      props->getMemberAsBool("validateJsonResponse", m_validateResponse);
      m_messagingList.clear();

      const Document &doc = props->getAsJson();
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
      
      val = Pointer("/managementQueueSize").Get(doc);
      if (val) {
        m_managementQueueSize = val->Get<size_t>();
      }
      val = Pointer("/networkQueueSize").Get(doc);
      if (val) {
        m_networkQueueSize = val->Get<size_t>();
      }
      TRC_INFORMATION(PAR(m_validateResponse));
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonSplitter instance deactivate" << std::endl <<
        "******************************"
      );

      delete m_networkQueue;

      TRC_FUNCTION_LEAVE("")
    }

    void attachInterface(shape::ILaunchService* iface)
    {
      m_iLaunchService = iface;
    }

    void detachInterface(shape::ILaunchService* iface)
    {
      if (m_iLaunchService == iface) {
        m_iLaunchService = nullptr;
      }
    }

    void attachInterface(iqrf::IMessagingService* iface)
    {
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

    void detachInterface(iqrf::IMessagingService* iface)
    {
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

  int JsonSplitter::getMsgQueueLen() const
  {
   return m_imp->getMsgQueueLen();
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
