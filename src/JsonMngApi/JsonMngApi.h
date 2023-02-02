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

#include "ILaunchService.h"
#include "IIqrfDpaService.h"
#include "IIqrfInfo.h"
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

		void attachInterface(IIqrfDpaService *iface);
		void detachInterface(IIqrfDpaService *iface);

		void attachInterface(IIqrfInfo *iface);
		void detachInterface(IIqrfInfo *iface);

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
