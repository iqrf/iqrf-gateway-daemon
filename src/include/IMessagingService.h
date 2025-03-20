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
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <stdexcept>

#ifdef IMessagingService_EXPORTS
#define IMessagingService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IMessagingService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class IMessaging
 /// \brief IMessaging interface
 /// \details
 /// Provides interface for sending/receiving message from/to general communication interface
  class IMessagingService_DECLSPEC IMessagingService
  {
  public:

    /// Incoming message handler functional type
    typedef std::function<void(const MessagingInstance&, const std::vector<uint8_t>&)> MessageHandlerFunc;

    /// \brief Register message handler
    /// \param [in] hndl registering handler function
    /// \details
    /// Whenever a message is received it is passed to the handler function. It is possible to register
    /// just one handler
    virtual void registerMessageHandler(MessageHandlerFunc hndl) = 0;

    /// \brief Unregister message handler
    /// \details
    /// If the handler is not required anymore, it is possible to unregister via this method.
    virtual void unregisterMessageHandler() = 0;

    /// \brief send message
    /// \param [in] msg message to be sent
    /// \details
    /// The message is send outside
    virtual void sendMessage(const MessagingInstance &messaging, const std::basic_string<uint8_t> & msg) = 0;
    virtual bool acceptAsyncMsg() const = 0;
    virtual const MessagingInstance &getMessagingInstance() const = 0;

    inline virtual ~IMessagingService() {};
  };
}
