/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#include "ComIqrfStandard.h"
#include "DpaPerExceptions.h"
#include "FakeTransactionResult.h"
#include "IIqrfDb.h"
#include "IJsRenderService.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "IMessagingService.h"
#include "ITraceService.h"
#include "EnumUtils.h"
#include "MessageTypes.h"
#include "RawAny.h"
#include "Trace.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include <algorithm>
#include <fstream>
#include <map>

/// iqrf namespace
namespace iqrf {

	class JsonDpaApiIqrfStandard {
	public:
		/**
		 * Constructor
		 */
		JsonDpaApiIqrfStandard();

		/**
		 * Destructor
		 */
		virtual ~JsonDpaApiIqrfStandard();

		/**
		 * Activate component instance
		 * @param props Instance properteis
		 */
		void activate(const shape::Properties *props = 0);

		/**
		 * Modify component instance members
		 * @param props Instance properties
		 */
		void modify(const shape::Properties *props);

		/**
		 * Deactivate component instance
		 */
		void deactivate();

		/**
		 * Attach DB service interface
		 * @param iface DB service interface
		 */
		void attachInterface(IIqrfDb *iface);

		/**
		 * Detach DB service interface
		 * @param iface DB service interface
		 */
		void detachInterface(IIqrfDb *iface);

		/**
		 * Attach DPA service interface
		 * @param iface DPA service interface
		 */
		void attachInterface(IIqrfDpaService *iface);

		/**
		 * Detach DPA service interface
		 * @param iface DPA service interface
		 */
		void detachInterface(IIqrfDpaService *iface);

		/**
		 * Attach JS render service interface
		 * @param iface JS render service interface
		 */
		void attachInterface(IJsRenderService *iface);

		/**
		 * Detach JS render service interface
		 * @param iface JS render service interface
		 */
		void detachInterface(IJsRenderService *iface);

		/**
		 * Attach Splitter service interface
		 * @param iface Splitter service interface
		 */
		void attachInterface(IMessagingSplitterService *iface);

		/**
		 * Detach Splitter service interface
		 * @param iface Splitter service interface
		 */
		void detachInterface(IMessagingSplitterService *iface);

		/**
		 * Attach Tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService *iface);

		/**
		 * Detach Tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService *iface);

	private:
		/**
		 * Handles DPA messages
		 * @param messagingId Messaging ID
		 * @param msgType Message type
		 * @param doc Request document
		 */
		void handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc);

		/**
		 * Handles asynchronous DPA messages
		 * @param dpaMessage DPA message
		 */
		void handleAsyncMsg(const DpaMessage &dpaMessage);

		/**
		 * Converts raw HDP request string to vector of DPA message bytes
		 * @param nadr Device address
		 * @param hwpid Device HWPID
		 * @param hdpRequest Request string
		 * @return Vector of DPA message bytes
		 */
		std::vector<uint8_t> hdpToDpa(uint8_t nadr, uint16_t hwpid, const std::string &hdpRequest);

		/**
		 * Converts DPA message to raw HDP response string
		 * @param nadr Device address
		 * @param hwpid Device HWPID
		 * @param rcode Return code
		 * @param dpaResponse DPA response
		 * @param hdpRequest Original HDP request
		 * @return Raw HDP response string
		 */
		std::string dpaToHdp(int &nadr, int &hwpid, int &rcode, const std::vector<uint8_t> &dpaResponse, const std::string &hdpRequest);

		/**
		 * Convert JSON object to string representation
		 * @param val JSON object or property
		 * @return String representation of JSON object
		 */
		std::string jsonToStr(const rapidjson::Value *val);

		/// Database service
		IIqrfDb *m_dbService = nullptr;
		/// DPA service
		IIqrfDpaService *m_dpaService = nullptr;
		/// JS render service
		IJsRenderService *m_jsRenderService = nullptr;
		/// Splitter service
		IMessagingSplitterService *m_splitterService = nullptr;
		/// Component instance name
		std::string m_instance;
		/// Transaction mutex
		std::mutex m_transactionMutex;
		/// Current transaction
		std::shared_ptr<IDpaTransaction2> m_transaction;
		/// Vector of handled DPA messages
		std::vector<std::string> m_filters = {
			"iqrfEmbed",
			"iqrfLight",
			"iqrfSensor",
			"iqrfBinaryoutput",
			"iqrfDali"
		};
	};
}
