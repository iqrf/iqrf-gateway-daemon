/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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

#include "EnumStringConvertor.h"
#include "ShapeDefines.h"
#include <string>
#include <vector>
#include <functional>

#ifdef IUdpConnectorService_EXPORTS
#define IUdpConnectorService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IUdpConnectorService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class IMessaging
 /// \brief IMessaging interface
 /// \details
 /// Provides interface for sending/receiving message from/to general communication interface
  class IUdpConnectorService_DECLSPEC IUdpConnectorService
  {
  public:
    /// \brief operational mode
    /// \details
    /// Unknown no change
    /// Operational is used for normal work
    /// Service the only UDP Messaging is used to communicate with IQRF IDE
    /// Forwarding normal work but all DPA messages are forwarded to IQRF IDE to me monitored there
    enum class Mode {
      Unknown,
      Operational,
      Service,
      Forwarding
    };

    /// \brief switch operational mode
    /// \param [in] mode operational mode to switch
    /// \details
    /// \details
    /// Operational is used for normal work
    /// Service the only UDP Messaging is used to communicate with IQRF IDE
    /// Forwarding normal work but all DPA messages are forwarded to IQRF IDE to me monitored there
    virtual void setMode(Mode mode) = 0;

    virtual Mode getMode() const = 0;
    virtual void registerModeSetCallback(const std::string &instanceId, std::function<void()> callback) = 0;
    virtual void unregisterModeSetCallback(const std::string &instanceId) = 0;

    inline virtual ~IUdpConnectorService() {};
  };

  class ModeConvertTable
  {
  public:
    static const std::vector<std::pair<IUdpConnectorService::Mode, std::string>>& table()
    {
      static std::vector <std::pair<IUdpConnectorService::Mode, std::string>> table = {
        { IUdpConnectorService::Mode::Unknown, "unknown" },
        { IUdpConnectorService::Mode::Forwarding, "forwarding" },
        { IUdpConnectorService::Mode::Operational, "operational" },
        { IUdpConnectorService::Mode::Service, "service" }
      };
      return table;
    }
    static IUdpConnectorService::Mode defaultEnum()
    {
      return IUdpConnectorService::Mode::Unknown;
    }
    static const std::string& defaultStr()
    {
      static std::string u("unknown");
      return u;
    }
  };

  typedef shape::EnumStringConvertor<IUdpConnectorService::Mode, ModeConvertTable> ModeStringConvertor;

}
