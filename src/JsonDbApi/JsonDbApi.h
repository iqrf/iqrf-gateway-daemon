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

#include "IIqrfDb.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include "Trace.h"

#include "Messages/EnumerateMsg.h"
#include "Messages/GetBinaryOutputsMsg.h"
#include "Messages/GetDalisMsg.h"
#include "Messages/GetDevicesMsg.h"
#include "Messages/GetNetworkTopologyMsg.h"
#include "Messages/GetLightsMsg.h"
#include "Messages/GetSensorsMsg.h"
#include "Messages/ResetMsg.h"
#include "Messages/GetDeviceMetadataMsg.h"
#include "Messages/SetDeviceMetadataMsg.h"

#include <memory>
#include <vector>

namespace iqrf {
	/**
	 * IQRF DB Api
	 */
	class JsonDbApi {
	public:
		/**
		 * Constructor
		 */
		JsonDbApi();

		/**
		 * Destructor
		 */
		virtual ~JsonDbApi();

		/**
		 * Activates and configures component instance
		 * @param props Component instance properties
		 */
		void activate(const shape::Properties *props = 0);

		/**
		 * Updates component instance properties
		 * @param props Component instance properties
		 */
		void modify(const shape::Properties *props);

		/**
		 * Cleans up and deactivates component
		 */
		void deactivate();

		/**
		 * Attaches IQRF DB interface
		 * @param iface IQRF DB interface
		 */
		void attachInterface(IIqrfDb *iface);

		/**
		 * Detaches IQRF DB interface
		 * @param iface IQRF DB interface
		 */
		void detachInterface(IIqrfDb *iface);

		/**
		 * Attaches Splitter service interface
		 * @param iface Splitter service interface
		 */
		void attachInterface(IMessagingSplitterService *iface);

		/**
		 * Detaches Splitter service interface
		 * @param iface Splitter service interface
		 */
		void detachInterface(IMessagingSplitterService *iface);

		/**
		 * Attaches Tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService *iface);

		/**
		 * Dettaches Tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService *iface);
	private:
		/**
		 * IQRF DB API request message handler
		 * @param messagingId Messaging ID
		 * @param msgType Request message type
		 * @param request Request message document
		 */
		void handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document request);

		/**
		 * Sends enumeration process response
		 * @param progress Enumeration progress, step and details
		 */
		void sendEnumerationResponse(IIqrfDb::EnumerationProgress progress);

		/**
		 * Sends asynchronous enumeration finish response
		 * @param progress Enumeration progress, step and details
		 */
		void sendAsyncEnumerationFinishResponse(IIqrfDb::EnumerationProgress progress);

		/// Database service
		IIqrfDb *m_dbService = nullptr;
		/// Splitter service
		IMessagingSplitterService *m_splitterService = nullptr;
		/// Vector of IQRF DB message types
		std::vector<std::string> m_messageTypes = {
			"iqrfDb_GetBinaryOutput",
			"iqrfDb_GetDalis",
			"iqrfDb_GetDevices",
			"iqrfDb_GetNetworkTopology",
			"iqrfDb_GetLights",
			"iqrfDb_GetSensors",
			"iqrfDb_Enumerate",
			"iqrfDb_Reset",
			"iqrfDb_GetDeviceMetadata",
			"iqrfDb_SetDeviceMetadata"
		};
		/// Component instance name
		std::string m_instance;
		/// Enumeration message pointer for progress reporting
		std::unique_ptr<EnumerateMsg> m_enumerateMsg;
		/// Enumeration message reporting mutex
		std::mutex m_enumerateMutex;
	};
}