#define IMessagingSplitterService_EXPORTS

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

#include <fstream>
#include <algorithm>
#include <vector>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__JsonSplitter.hxx"

TRC_INIT_MODULE(iqrf::JsonSplitter);

using namespace rapidjson;

namespace iqrf {
  class JsonSplitter::Imp {
  private:
    std::string m_schemesDir;

    std::map<std::string, IMessagingService*> m_iMessagingServiceMap;
    std::map<std::string, MsgType> m_filteredMessageHandlerFuncMap;

    std::map<std::string, rapidjson::SchemaDocument> m_validatorMap;

    std::string getKey(const MsgType& msgType) const
    {
      std::ostringstream os;
      os << msgType.m_type << '.' << msgType.m_major << '.' << msgType.m_minor << '.' << msgType.m_micro;
      return os.str();
    }

  public:
    void sendMessage(const std::string& messagingId, rapidjson::Document doc) const
    {
      auto found = m_iMessagingServiceMap.find(messagingId);
      if (found != m_iMessagingServiceMap.end()) {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        found->second->sendMessage(std::basic_string<uint8_t>((uint8_t*)buffer.GetString(), buffer.GetSize()));
      }
      else {
        TRC_WARNING("Cannot find required: " << PAR(messagingId))
      }
    }

    void registerFilteredMsgHandler(const std::vector<MsgType>& msgTypes)
    {
      for (auto & msgType : msgTypes) {
        m_filteredMessageHandlerFuncMap.insert(std::make_pair(getKey(msgType), msgType));
      }
    }

    void unregisterFilteredMsgHandler(const std::vector<MsgType>& msgTypes)
    {
      for (auto & msgType : msgTypes) {
        m_filteredMessageHandlerFuncMap.erase(getKey(msgType));
      }
    }

    void validate(const IMessagingSplitterService::MsgType & msgType, const Document& doc) const
    {
      TRC_FUNCTION_ENTER(PAR(msgType.m_type))
      auto found = m_validatorMap.find(getKey(msgType));
      if (found != m_validatorMap.end()) {
        SchemaValidator validator(found->second);
        if(false) {
        //if (!doc.Accept(validator)) {
          // Input JSON is invalid according to the schema
          // Output diagnostic information
          StringBuffer sb;
          validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
          TRC_WARNING("Invalid schema: " << sb.GetString());
          TRC_WARNING("Invalid keyword: " << validator.GetInvalidSchemaKeyword());
          sb.Clear();
          validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
          TRC_WARNING("Invalid document: " << sb.GetString());
          //TODO validation error handling => send back an error JSON with details
          THROW_EXC_TRC_WAR(std::logic_error, "Invalid");
        }
        TRC_DEBUG("OK");
      }
      else {
        //TODO why
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot find validator");
      }
      TRC_FUNCTION_LEAVE("")
    }

    void handleMessageFromMessaging(const std::string& messagingId, const std::vector<uint8_t>& message) const
    {
      using namespace rapidjson;

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

        std::string mType;
        std::string ver;
        int major = 1;
        int minor = 0;
        int micro = 0;

        // get message type
        if (Value* mTypeVal = Pointer("/mType").Get(doc)) {
          mType = mTypeVal->GetString();
        }
        else {
          //defaulted to support daemon V1 messages
          mType = "dpa-V1";
        }

        // get version
        if (Value* verVal = Pointer("/ver").Get(doc)) {
          ver = verVal->GetString();
          std::replace(ver.begin(), ver.end(), '.', ' ');
          std::istringstream istr(ver);
          istr >> major >> minor >> micro;
        }

        MsgType msgType(mType, major, minor, micro);

        validate(msgType, doc);

        auto found = m_filteredMessageHandlerFuncMap.find(getKey(msgType));
        if (found != m_filteredMessageHandlerFuncMap.end()) {
          found->second.m_handlerFunc(messagingId, found->second, std::move(doc));
        }
        else {
          //TODO parse error handling => send back an error JSON with details
          THROW_EXC_TRC_WAR(std::logic_error, "Unsupported: " << NAME_PAR(mType.version, getKey(msgType)));
        }

      }
      catch (std::logic_error &e) {
        //TODO send back parse error
        //TODO mtx?
        auto found = m_iMessagingServiceMap.find(messagingId);
        if (found != m_iMessagingServiceMap.end()) {
          std::string what(e.what());
          std::basic_string<uint8_t> bwhat((uint8_t*)what.c_str(), what.size());
          found->second->sendMessage(bwhat);
        }
      }
    }

#ifdef SHAPE_PLATFORM_WINDOWS
    std::vector<std::string> getConfigFiles(const std::string schemesDir)
    {
      WIN32_FIND_DATA fid;
      HANDLE found = INVALID_HANDLE_VALUE;

      std::vector<std::string> fileVect;
      std::string sdirect(schemesDir);
      sdirect.append("/*request.json");

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
    std::vector<std::string> getConfigFiles(const std::string schemesDir)
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

    void loadJsonSchemes(const std::string sdir)
    {
      TRC_FUNCTION_ENTER(PAR(sdir));

      std::vector<std::string> files = getConfigFiles(sdir);

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

          // preparse key
          std::string mType;
          std::string ver;
          int major = 1;
          int minor = 0;
          int micro = 0;

          // get message type
          if (Value* mTypeVal = Pointer("/properties/mType/enum/0").Get(sd)) {
            mType = mTypeVal->GetString();
          }
          else {
            //defaulted to support daemon V1 messages
            THROW_EXC_TRC_WAR(std::logic_error, "Invalid schema: " << PAR(fname));
          }

          // get version
          if (Value* verVal = Pointer("/properties/ver/enum/0").Get(sd)) {
            ver = verVal->GetString();
            std::replace(ver.begin(), ver.end(), '.', ' ');
            std::istringstream istr(ver);
            istr >> major >> minor >> micro;
          }
          else {
            //default
            major = 1; minor = 0; micro = 0;
          }

          MsgType msgType(mType, major, minor, micro);

          SchemaDocument schema(sd);
          m_validatorMap.insert(std::make_pair(getKey(msgType), std::move(schema)));
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

      if (shape::Properties::Result::ok == props->getMemberAsString("SchemesDir", m_schemesDir)) {
        TRC_INFORMATION("loading schemes from: " << PAR(m_schemesDir));
        loadJsonSchemes(m_schemesDir);
      }

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
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
    }

    void attachInterface(iqrf::IMessagingService* iface)
    {
      //TODO shall be targeted only to JSON content or "iqrf-daemon-api"
      m_iMessagingServiceMap.insert(std::make_pair(iface->getName(), iface));
      iface->registerMessageHandler([&](const std::string& messagingId, const std::vector<uint8_t>& message)
      {
        handleMessageFromMessaging(messagingId, message);
      });
    }

    void detachInterface(iqrf::IMessagingService* iface)
    {
      //TODO mtx?
      auto found = m_iMessagingServiceMap.find(iface->getName());
      if (found != m_iMessagingServiceMap.end() && found->second == iface) {
        iface->unregisterMessageHandler();
        m_iMessagingServiceMap.erase(found);
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

  void JsonSplitter::registerFilteredMsgHandler(const std::vector<MsgType>& msgTypes)
  {
    m_imp->registerFilteredMsgHandler(msgTypes);
  }

  void JsonSplitter::unregisterFilteredMsgHandler(const std::vector<MsgType>& msgTypes)
  {
    m_imp->unregisterFilteredMsgHandler(msgTypes);
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
