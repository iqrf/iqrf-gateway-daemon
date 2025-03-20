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

#include "ILaunchService.h"
#include "IIqrfDpaService.h"
#include "IIqrfDb.h"
#include "ISchedulerService.h"
#include "IJsCacheService.h"
#include "IUdpConnectorService.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <map>

namespace iqrf {
	class JsonMngApi {
	public:
		JsonMngApi();
		virtual ~JsonMngApi();

		void activate(const shape::Properties *props = 0);
		void deactivate();
		void modify(const shape::Properties *props);

		void attachInterface(shape::ILaunchService *iface);
		void detachInterface(shape::ILaunchService *iface);

		void attachInterface(IIqrfDb *iface);
		void detachInterface(IIqrfDb *iface);

		void attachInterface(IIqrfDpaService *iface);
		void detachInterface(IIqrfDpaService *iface);

		void attachInterface(ISchedulerService *iface);
		void detachInterface(ISchedulerService *iface);

		void attachInterface(IJsCacheService *iface);
		void detachInterface(IJsCacheService *iface);

		void attachInterface(IUdpConnectorService *iface);
		void detachInterface(IUdpConnectorService *iface);

		void attachInterface(IMessagingSplitterService *iface);
		void detachInterface(IMessagingSplitterService *iface);

		void attachInterface(shape::ITraceService *iface);
		void detachInterface(shape::ITraceService *iface);
	private:
		class Imp;
		Imp *m_imp;
	};
}
