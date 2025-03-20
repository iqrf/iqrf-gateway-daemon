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
