#define IMessagingSplitterService_EXPORTS

#include "JsonSplitter.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "JsonUtils.h"
#include "Trace.h"

#include <algorithm>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__JsonSplitter.hxx"

TRC_INIT_MODULE(iqrf::JsonSplitter);

namespace iqrf {
  JsonSplitter::JsonSplitter()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  JsonSplitter::~JsonSplitter()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  void JsonSplitter::sendMessage(const std::string& messagingId, rapidjson::Document doc) const
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

  void JsonSplitter::registerFilteredMsgHandler(const std::list<std::string>& msgTypes, FilteredMessageHandlerFunc handler)
  {
    for (auto & msgType : msgTypes) {
      m_filteredMessageHandlerFuncMap.insert(std::make_pair(msgType, handler));
    }
  }

  void JsonSplitter::unregisterFilteredMsgHandler(const std::list<std::string>& msgTypes)
  {
    for (auto & msgType : msgTypes) {
      m_filteredMessageHandlerFuncMap.erase(msgType);
    }
  }

  void JsonSplitter::handleMessageFromMessaging(const std::string& messagingId, const std::vector<uint8_t>& message) const
  {
    std::string str((char*)message.data(), message.size());
    rapidjson::StringStream sstr(str.data());
    rapidjson::Document doc;
    doc.ParseStream(sstr);

    try {
      if (doc.HasParseError()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " << NAME_PAR(emsg, doc.GetParseError()) <<
          NAME_PAR(eoffset, doc.GetErrorOffset()));
      }

      jutils::assertIsObject("", doc);
      std::string msgType = jutils::getMemberAs<std::string>("mType", doc);

      auto found = m_filteredMessageHandlerFuncMap.find(msgType);
      if (found != m_filteredMessageHandlerFuncMap.end()) {
        found->second(messagingId, msgType, std::move(doc));
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Unsupported: " << NAME_PAR(mType, msgType));
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

  void JsonSplitter::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "JsonSplitter instance activate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void JsonSplitter::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "JsonSplitter instance deactivate" << std::endl <<
      "******************************"
    );
    TRC_FUNCTION_LEAVE("")
  }

  void JsonSplitter::modify(const shape::Properties *props)
  {
  }

  void JsonSplitter::attachInterface(iqrf::IMessagingService* iface)
  {
    //TODO shall be targeted only to JSON content or "iqrf-daemon-api"
    m_iMessagingServiceMap.insert(std::make_pair(iface->getName(), iface));
    iface->registerMessageHandler([&](const std::string& messagingId, const std::vector<uint8_t>& message)
    {
      handleMessageFromMessaging(messagingId, message);
    });
  }

  void JsonSplitter::detachInterface(iqrf::IMessagingService* iface)
  {
    //TODO mtx?
    auto found = m_iMessagingServiceMap.find(iface->getName());
    if (found != m_iMessagingServiceMap.end() && found->second == iface) {
      iface->unregisterMessageHandler();
      m_iMessagingServiceMap.erase(found);
    }
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
