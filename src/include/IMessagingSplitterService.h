#pragma once

#include "ShapeDefines.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <string>
#include <vector>
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
    class MsgType;

    /// Incoming message handler functional type
    /// 1st parameter is messagingId, 2nd parameter is message
    typedef std::function<void(const std::string & messagingId, const MsgType & msgType, rapidjson::Document doc)> FilteredMessageHandlerFunc;

    class MsgType {
    public:
      MsgType(const std::string mtype, int major, int minor, int micro)
        :m_type(mtype), m_major(major), m_minor(minor), m_micro(micro)
      {}
      MsgType(const std::string mtype, int major, int minor, int micro, const std::string& possibleDriverFunction)
        :m_type(mtype), m_major(major), m_minor(minor), m_micro(micro), m_possibleDriverFunction(possibleDriverFunction)
      {}
      std::string m_type;
      int m_major = 1;
      int m_minor = 0;
      int m_micro = 0;
      std::string m_possibleDriverFunction;
      FilteredMessageHandlerFunc m_handlerFunc;
    };

    virtual void sendMessage(const std::string& messagingId, rapidjson::Document doc) const = 0;
    virtual void sendMessageExt(const std::string& messagingId, const int nodeAdr, rapidjson::Document doc) const = 0;
    virtual void registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, FilteredMessageHandlerFunc handlerFunc) = 0;
    virtual void unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters) = 0;
    virtual int getMsgQueueLen() const = 0;

    virtual ~IMessagingSplitterService() {}
  };
}
