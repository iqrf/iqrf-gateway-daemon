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

#include "IPingService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "ITraceService.h"
#include <string>

/// iqrf namespace
namespace iqrf {
	/// \class PingService
	/// \brief Implementation of IPingService
	class PingService : public IPingService {
	public:
		/**
		 * Constructor
		 */
		PingService();

		/**
		 * Destructor
		 */
		virtual ~PingService();

		void activate(const shape::Properties *props = 0);
		void modify(const shape::Properties *props);
		void deactivate();

		void attachInterface(iqrf::IIqrfDpaService *iface);
		void detachInterface(iqrf::IIqrfDpaService *iface);

		void attachInterface(IMessagingSplitterService *iface);
		void detachInterface(IMessagingSplitterService *iface);

		void attachInterface(shape::ITraceService *iface);
		void detachInterface(shape::ITraceService *iface);
	private:
		class Imp;
		Imp *m_imp;
	};
}
