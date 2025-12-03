/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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
#include "MessagingCommon.h"
#include "ShapeDefines.h"
#include <string>
#include <vector>
#include <functional>

#ifdef IModeService_EXPORTS
#define IModeService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IModeService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class IMessaging
  /// \brief IMessaging interface
  /// \details
  /// Provides interface for management of daemon mode and direct communication with IQRF interface
  class IModeService_DECLSPEC IModeService
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

    /**
     * Service mode type
     */
    enum class ServiceModeType {
      None,       //< Inactive
      Legacy,     //< Legacy mode for UDP
      New,        //< New mode for WS
    };

    virtual Mode getMode() const = 0;
    virtual ServiceModeType getServiceModeType() const = 0;
    virtual void registerModeSetCallback(const std::string &instanceId, std::function<void()> callback) = 0;
    virtual void unregisterModeSetCallback(const std::string &instanceId) = 0;
    virtual void clientDisconnected(const MessagingInstance& messaging) = 0;

    inline virtual ~IModeService() {};
  };

  class ModeConvertTable
  {
  public:
    static const std::vector<std::pair<IModeService::Mode, std::string>>& table()
    {
      static std::vector <std::pair<IModeService::Mode, std::string>> table = {
        { IModeService::Mode::Unknown, "unknown" },
        { IModeService::Mode::Forwarding, "forwarding" },
        { IModeService::Mode::Operational, "operational" },
        { IModeService::Mode::Service, "service" }
      };
      return table;
    }
    static IModeService::Mode defaultEnum()
    {
      return IModeService::Mode::Unknown;
    }
    static const std::string& defaultStr()
    {
      static std::string u("unknown");
      return u;
    }
  };

  typedef shape::EnumStringConvertor<IModeService::Mode, ModeConvertTable> ModeStringConvertor;

}
