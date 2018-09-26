#define IMessagingSplitterService_EXPORTS

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

TRC_INIT_MODULE(iqrf::JsonSplitter);

using namespace rapidjson;

namespace iqrf {
  class MessageErrorMsg : public ApiMsg
  {
  public:
    MessageErrorMsg() = delete;
    MessageErrorMsg(const std::string msgId, const std::string wrongMsg, const std::string errorStr)
      :ApiMsg("messageError", msgId, true)
      ,m_wrongMsg(wrongMsg)
      ,m_errorStr(errorStr)
    {
    }

    virtual ~MessageErrorMsg()
    {
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      Pointer("/data/rsp/wrongMsg").Set(doc, m_wrongMsg);
      Pointer("/data/rsp/errorStr").Set(doc, m_errorStr);
      setStatus("err",-1);
    }

  private:
    std::string m_wrongMsg;
    std::string m_errorStr;
  };

  class JsonSplitter::Imp {
  private:
    bool m_validateResponse = true;
    std::string m_schemesDir;

    shape::ILaunchService* m_iLaunchService = nullptr;
    mutable std::mutex m_iMessagingServiceMapMux;
    std::map<std::string, IMessagingService*> m_iMessagingServiceMap;
    std::set<IMessagingService*> m_iMessagingServiceSetAcceptAsync;
    std::map<std::string, FilteredMessageHandlerFunc > m_filterMessageHandlerFuncMap;

    std::map<std::string, rapidjson::SchemaDocument> m_validatorMapRequest;
    std::map<std::string, rapidjson::SchemaDocument> m_validatorMapResponse;
    std::map<std::string, MsgType> m_msgTypeToHandle; //TODO temporary

    typedef std::pair<std::string, std::vector<uint8_t>> MsgIdMsg; //pairing messagingId with msg 

    TaskQueue<MsgIdMsg>* m_splitterMessageQueue = nullptr;

    std::string getKey(const MsgType& msgType) const
    {
      std::ostringstream os;
      os << msgType.m_type << '.' << msgType.m_major << '.' << msgType.m_minor << '.' << msgType.m_micro;
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
      int micro = 0;

      // get message type
      if (const Value* mTypeVal = Pointer("/mType").Get(doc)) {
        mType = mTypeVal->GetString();
      }
      else {
        //defaulted to support daemon V1 messages
        mType = "dpaV1";
      }

      // get version
      if (const Value* verVal = Pointer("/ver").Get(doc)) {
        ver = verVal->GetString();
        std::replace(ver.begin(), ver.end(), '.', ' ');
        std::istringstream istr(ver);
        istr >> major >> minor >> micro;
      }

      return MsgType(mType, major, minor, micro);
    }

    void sendMessage(const std::string& messagingId, rapidjson::Document doc) const
    {
      using namespace rapidjson;
      
      TRC_INFORMATION("Outcomming message:\n"
        << NAME_PAR(Messaging ID, messagingId)
        << "\n"
        << NAME_PAR(Sent message, JsonToStr(doc))
      )

      MsgType msgType = getMessageType(doc);
      if (msgType.m_type != "dpaV1") { // dpaV1 is default legacy support
        auto foundType = m_msgTypeToHandle.find(getKey(msgType));
        if (foundType == m_msgTypeToHandle.end()) {
          THROW_EXC_TRC_WAR(std::logic_error, "Unsupported: " << NAME_PAR(mType, msgType.m_type));
        }

        msgType = foundType->second;
        if (m_validateResponse) {
          validate(msgType, doc, m_validatorMapResponse, "response");
        }
      }
      
      StringBuffer buffer;
      PrettyWriter<StringBuffer> writer(buffer);
      doc.Accept(writer);

      if (!messagingId.empty()) {
        std::string messagingId2(messagingId);
        if (std::string::npos != messagingId2.find_first_of('/')) {
          //preparse messageId to remove possible optional appended topic
          //we need just clean massaging name to find in map
          std::string buf(messagingId2);
          std::replace(buf.begin(), buf.end(), '/', ' ');
          std::istringstream is(buf);
          is >> messagingId2;
        }

        std::unique_lock<std::mutex> lck(m_iMessagingServiceMapMux);
        auto found = m_iMessagingServiceMap.find(messagingId2);
        if (found != m_iMessagingServiceMap.end()) {
          found->second->sendMessage(messagingId, std::basic_string<uint8_t>((uint8_t*)buffer.GetString(), buffer.GetSize()));
          TRC_INFORMATION("Outcomming message successfully sent.");
        }
        else {
          TRC_WARNING("Cannot find required: " << PAR(messagingId))
        }
      }
      else {
        std::unique_lock<std::mutex> lck(m_iMessagingServiceMapMux);
        for (auto m : m_iMessagingServiceSetAcceptAsync) {
          m->sendMessage(messagingId, std::basic_string<uint8_t>((uint8_t*)buffer.GetString(), buffer.GetSize()));
          TRC_INFORMATION("Outcomming message successfully sent.");
        }
      }
    }

    void registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, FilteredMessageHandlerFunc handlerFunc)
    {
      for (const auto & ft : msgTypeFilters) {
        m_filterMessageHandlerFuncMap.insert(std::make_pair(ft, handlerFunc));
      }
    }

    void unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters)
    {
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

    void handleMessageFromMessaging(const std::string& messagingId, const std::vector<uint8_t>& message) const
    {
      TRC_FUNCTION_ENTER(PAR(messagingId));

      std::string msgStr((char*)message.data(), message.size());
      
      TRC_INFORMATION("Incomming message:\n"
        << NAME_PAR(Messaging ID, messagingId)
        << "\n"
        << NAME_PAR(Message, msgStr)
        );

      int queueLen = -1;
      if (m_splitterMessageQueue) {
        queueLen = m_splitterMessageQueue->pushToQueue(std::make_pair(messagingId, message));
      }
      else {
        TRC_WARNING("Not activated yet => message is droped ");
      }
      TRC_FUNCTION_LEAVE(PAR(queueLen))
    }

    void handleMessageFromSplitterQueue(const std::string& messagingId, const std::vector<uint8_t>& message) const
    {
      using namespace rapidjson;

      std::string msgId("undefined");
      std::string str((char*)message.data(), message.size());
      StringStream sstr(str.data());
      Document doc;
      doc.ParseStream(sstr);

      try {
        if (doc.HasParseError()) {
          //TODO parse error handling => send back an error JSON with details
          THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " << NAME_PAR(emsg, doc.GetParseError()) <<
            NAME_PAR(eoffset, doc.GetErrorOffset()));
        }

        MsgType msgType = getMessageType(doc);

        if (msgType.m_type != "dpaV1") { // dpaV1 is default legacy support
          auto foundType = m_msgTypeToHandle.find(getKey(msgType));
          if (foundType == m_msgTypeToHandle.end()) {
            THROW_EXC_TRC_WAR(std::logic_error, "Unsupported: " << NAME_PAR(mType, msgType.m_type) << NAME_PAR(key, getKey(msgType)));
          }

          const std::string REQS("request");
          msgType = foundType->second;
          validate(msgType, doc, m_validatorMapRequest, REQS);
        }

        bool found = false;
        std::map<std::string, FilteredMessageHandlerFunc > bestFitMap;
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
            selected(messagingId, msgType, std::move(doc));
            TRC_INFORMATION("Incomming message successfully handled.");
          }
          catch (std::exception &e) {
            THROW_EXC_TRC_WAR(std::logic_error, "Unhandled exception: " << e.what());
          }
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Unsupported: " << NAME_PAR(mType.version, getKey(msgType)));
        }

      }
      catch (std::logic_error &e) {
        TRC_WARNING("Error while handling incomming message:" << e.what());

        Document rspDoc;
        MessageErrorMsg msg(msgId, str, e.what());
        msg.createResponse(rspDoc);
        try {
          sendMessage(messagingId, std::move(rspDoc));
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
          TRC_DEBUG ("Add: " << NAME_PAR(key, getKey(msgType)) << PAR(msgType.m_type) << " ver" << msgType.m_major << '.' << msgType.m_minor << '.' << msgType.m_micro << " drv:" <<
            msgType.m_possibleDriverFunction)
        }
        catch (std::exception & e) {
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

      m_splitterMessageQueue = shape_new TaskQueue<MsgIdMsg>([&](const MsgIdMsg& msgIdMsg) {
        handleMessageFromSplitterQueue(msgIdMsg.first, msgIdMsg.second);
      });

      props->getMemberAsBool("validateJsonResponse", m_validateResponse);
      TRC_INFORMATION(PAR(m_validateResponse));
      m_schemesDir = m_iLaunchService->getDataDir() + "/apiSchemas";
      TRC_INFORMATION("loading schemes from: " << PAR(m_schemesDir));
      loadJsonSchemesRequest(m_schemesDir);

      m_splitterMessageQueue = shape_new TaskQueue<MsgIdMsg>([&](const MsgIdMsg& msgIdMsg) {
        handleMessageFromSplitterQueue(msgIdMsg.first, msgIdMsg.second);
      });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonSplitter instance deactivate" << std::endl <<
        "******************************"
      );

      delete m_splitterMessageQueue;

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
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
      m_iMessagingServiceMap.insert(std::make_pair(iface->getName(), iface));
      iface->registerMessageHandler([&](const std::string& messagingId, const std::vector<uint8_t>& message)
      {
        handleMessageFromMessaging(messagingId, message);
      });

      if (iface->acceptAsyncMsg()) {
        m_iMessagingServiceSetAcceptAsync.insert(iface);
      }
    }

    void detachInterface(iqrf::IMessagingService* iface)
    {
      std::unique_lock<std::mutex> lck(m_iMessagingServiceMapMux);
      {
        auto found = m_iMessagingServiceMap.find(iface->getName());
        if (found != m_iMessagingServiceMap.end() && found->second == iface) {
          iface->unregisterMessageHandler();
          m_iMessagingServiceMap.erase(found);
        }
      }
      {
        auto found = m_iMessagingServiceSetAcceptAsync.find(iface);
        if (found != m_iMessagingServiceSetAcceptAsync.end() && *found == iface) {
          m_iMessagingServiceSetAcceptAsync.erase(found);
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

  void JsonSplitter::sendMessage(const std::string& messagingId, rapidjson::Document doc) const
  {
    m_imp->sendMessage(messagingId, std::move(doc));
  }

  void JsonSplitter::registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, FilteredMessageHandlerFunc handlerFunc)
  {
    m_imp->registerFilteredMsgHandler(msgTypeFilters, handlerFunc);
  }

  void JsonSplitter::unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters)
  {
    m_imp->unregisterFilteredMsgHandler(msgTypeFilters);
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
