/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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

namespace iqrf {

  class IModeService {
  public:

    /// @brief Possible Daemon operating modes
    enum class Mode {
      Unknown,      //< Unknown mode
      Operational,  //< Used for normal operation
      Service,      //< Service mode for via UDP or WS
      Forwarding    //< Forwards messages to IQRF IDE via UDP
    };

    /// @brief Possible service mode types
    enum class ServiceModeType {
      None,       //< Inactive
      Legacy,     //< Legacy mode for UDP
      New,        //< New mode for WS
    };

    /**
     * Get current mode
     * @return `Mode` Mode
     */
    virtual Mode getMode() const = 0;

    /**
     * Get active service mode type (not active, legacy mode, or new mode)
     * @return `ServiceModeType` Service mode type
     */
    virtual ServiceModeType getServiceModeType() const = 0;

    /**
     * Registers a callback to execute on mode change
     *
     * @param instanceId ID of instance registering callback
     * @param callback Callback to execute
     */
    virtual void registerModeChangeCallback(const std::string &instanceId, std::function<void()> callback) = 0;

    /**
     * Unregisters a mode change callback by instance ID
     *
     * @param instanceId Instance ID
     */
    virtual void unregisterModeChangeCallback(const std::string &instanceId) = 0;

    /**
     * Checks if disconnected client owned service mode and exits out of service mode if needed
     *
     * @param messaging Messaging instance
     */
    virtual void processClientDisconnected(const MessagingInstance& messaging) = 0;

    inline virtual ~IModeService() {};
  };

  class ModeConvertTable {
  public:
    static const std::vector<std::pair<IModeService::Mode, std::string>>& table() {
      static std::vector <std::pair<IModeService::Mode, std::string>> table = {
        { IModeService::Mode::Unknown, "unknown" },
        { IModeService::Mode::Forwarding, "forwarding" },
        { IModeService::Mode::Operational, "operational" },
        { IModeService::Mode::Service, "service" }
      };
      return table;
    }

    static IModeService::Mode defaultEnum() {
      return IModeService::Mode::Unknown;
    }

    static const std::string& defaultStr() {
      static std::string u("unknown");
      return u;
    }
  };

  typedef shape::EnumStringConvertor<IModeService::Mode, ModeConvertTable> ModeStringConvertor;
}
