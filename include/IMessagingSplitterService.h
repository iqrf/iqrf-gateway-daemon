#pragma once

#include "ShapeDefines.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <string>
#include <list>
#include <functional>

#ifdef IMessagingSplitterService_EXPORTS
#define IMessagingSplitterService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IMessagingSplitterService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  ///// Incoming message handler functional type
  ///// 1st parameter is messagingId, 2nd parameter is message
  //typedef std::function<void(const std::string &, rapidjson::Document)> MessageHandlerFunc;

  class IMessagingSplitterService_DECLSPEC IMessagingSplitterService
  {
  public:
    /// Incoming message handler functional type
    /// 1st parameter is messagingId, 2nd parameter is message
    typedef std::function<void(const std::string & messagingId, const std::string & msgType, rapidjson::Document doc)> FilteredMessageHandlerFunc;

    virtual void sendMessage(const std::string& messagingId, rapidjson::Document doc) const = 0;
    virtual void registerFilteredMsgHandler(const std::list<std::string>& msgTypes, FilteredMessageHandlerFunc handler) = 0;
    virtual void unregisterFilteredMsgHandler(const std::list<std::string>& msgTypes) = 0;
    virtual ~IMessagingSplitterService() {}
  };
}
