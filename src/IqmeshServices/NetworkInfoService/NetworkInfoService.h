/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

#include "ComIqmeshNetworkInfo.h"
#include "IIqrfNetworkInfo.h"
#include "IMessagingSplitterService.h"
#include "INetworkInfoService.h"
#include "ITraceService.h"

#include "NetworkInfoResult.h"
#include "ShapeProperties.h"

/// iqrf namespace
namespace iqrf {
	/// NetworkInfoService class
	class NetworkInfoService : public INetworkInfoService {
	public:
		/// Service error codes
		enum ErrorCodes {
			serviceError = 1000,
			requestParseError = 1001,
			exclusiveAccessError = 1002,
		};

		/**
		 * Constructor
		 */
		NetworkInfoService();

		/**
		 * Destructor
		 */
		virtual ~NetworkInfoService();

		/**
		 * Initializes component
		 * @param props Component properties
		 */
		void activate(const shape::Properties *props = 0);

		/**
		 * Modifies component properties
		 * @param props Component properties
		 */
		void modify(const shape::Properties *props);

		/**
		 * Deactivates component
		 */
		void deactivate();

		/**
		 * Attaches IqrfNetworkInfo interface
		 * @param iface IqrfNetworkInfo interface
		 */
		void attachInterface(iqrf::IIqrfNetworkInfo *iface);

		/**
		 * Detaches IqrfNetworkInfo interface
		 * @param iface IqrfNetworkInfo interface
		 */
		void detachInterface(iqrf::IIqrfNetworkInfo *iface);

		/**
		 * Attaches splitter service interface
		 * @param iface Splitter service interface
		 */
		void attachInterface(IMessagingSplitterService *iface);

		/**
		 * Detaches splitter service interface
		 * @param iface Splitter service interface
		 */
		void detachInterface(IMessagingSplitterService *iface);

		/**
		 * Attaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService *iface);

		/**
		 * Detaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService *iface);
	private:
		/**
		 * Handles request from splitter
		 * @param messagingId Messaging ID
		 * @param msgType Message type
		 * @param doc request document
		 */
		void handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc);

		/// Message type
		const std::vector<std::string> m_mTypes = {
			"iqmeshNetwork_NetworkInfo"
		};
		/// Request parameters
		TNetworkInfoParams m_requestParams;
		/// IqrfNetworkInfo interface
		IIqrfNetworkInfo *m_networkInfo = nullptr;
		/// Splitter service
		IMessagingSplitterService *m_splitterService = nullptr;
	};
}
