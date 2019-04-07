/*
 * Copyright 2016-2017 MICRORISC s.r.o.
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

#pragma once

#include "ShapeDefines.h"
#include <string>
#include <vector>
#include <functional>

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
    typedef std::function<void(const std::string&, const std::vector<uint8_t>&)> MessageHandlerFunc;

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
    virtual void sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg) = 0;
    virtual const std::string & getName() const = 0;
    virtual bool acceptAsyncMsg() const = 0;

    inline virtual ~IMessagingService() {};
  };
}
