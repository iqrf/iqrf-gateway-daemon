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

#include "IIqrfDb.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include "Trace.h"

#include "Messages/EnumerateMsg.h"
#include "Messages/GetBinaryOutputsMsg.h"
#include "Messages/GetDeviceMsg.h"
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
		void handleMsg(const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document request);

		/**
		 * Sends enumeration error response
		 * @param error Enumeration error
		 * @param request Request message
		 */
		void sendEnumerationErrorResponse(const MessagingInstance &messaging, IIqrfDb::EnumerationError::Errors errCode, rapidjson::Document request);

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
			"iqrfDb_GetDevice",
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
