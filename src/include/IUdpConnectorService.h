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
