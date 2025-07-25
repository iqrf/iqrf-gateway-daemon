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

#include "IIqrfDb.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include "Trace.h"

#include "Messages/EnumerateMsg.h"

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
		 * @param messaging Messaging instance
		 * @param msgType Request message type
		 * @param request Request message document
		 */
		void handleMsg(const MessagingInstance& messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document request);

		/**
		 * Sends enumeration error response
		 * @param messaging Messaging ID
		 * @param error Enumeration error
		 * @param request Request message
		 */
		void sendEnumerationErrorResponse(const MessagingInstance& messaging, IIqrfDb::EnumerationError::Errors errCode, rapidjson::Document request);

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
			"iqrfDb_Enumerate",
			"iqrfDb_GetBinaryOutput",
			"iqrfDb_GetDalis",
			"iqrfDb_GetDevice",
			"iqrfDb_GetDeviceMetadata",
			"iqrfDb_GetDevices",
			"iqrfDb_GetNetworkTopology",
			"iqrfDb_GetLights",
			"iqrfDb_GetSensors",
			"iqrfDb_MetadataAnnotation",
			"iqrfDb_Reset",
			"iqrfDb_SetDeviceMetadata",
			// Legacy API messages
			"infoDaemon_Enumeration",
			"infoDaemon_GetBinaryOutputs",
			"infoDaemon_GetLights",
			"infoDaemon_GetMidMetaData",
			"infoDaemon_GetNodeMetaData",
			"infoDaemon_GetNodes",
			"infoDaemon_GetSensors",
			"infoDaemon_MidMetaDataAnnotate",
			"infoDaemon_Reset",
			"infoDaemon_SetMidMetaData",
			"infoDaemon_SetNodeMetaData"
		};
		/// Component instance name
		std::string m_instance;
		/// Enumeration message pointer for progress reporting
		std::unique_ptr<EnumerateMsg> m_enumerateMsg;
		/// Enumeration message reporting mutex
		std::mutex m_enumerateMutex;
	};
}
