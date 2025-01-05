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

#include "IMessagingService.h"

#ifdef IUdpMessagingService_EXPORTS
#define IUdpMessagingService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IUdpMessagingService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  /// Special iface type just for UDP communication not to mix with other IMessagingService ifaces
  class IUdpMessagingService_DECLSPEC IUdpMessagingService : public IMessagingService
  {
  public:
    virtual const std::string& getListeningIpAddress() const = 0;
    virtual unsigned short getListeningIpPort() const = 0;
    virtual const std::string& getListeningMacAddress() const = 0;
    inline virtual ~IUdpMessagingService() {};
  };
}
