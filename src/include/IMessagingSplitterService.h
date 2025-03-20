/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "MessagingCommon.h"
#include "ShapeDefines.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include <functional>
#include <list>
#include <string>
#include <vector>


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
    typedef std::function<void(const MessagingInstance &messaging, const MsgType & msgType, rapidjson::Document doc)> FilteredMessageHandlerFunc;

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

    virtual void sendMessage(const MessagingInstance &messaging, rapidjson::Document doc) const = 0;
    virtual void sendMessage(const std::list<MessagingInstance> &messagings, rapidjson::Document doc) const = 0;
    virtual void registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, FilteredMessageHandlerFunc handlerFunc) = 0;
    virtual void unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters) = 0;
    virtual int getMsgQueueLen() const = 0;

    virtual ~IMessagingSplitterService() {}
  };
}
