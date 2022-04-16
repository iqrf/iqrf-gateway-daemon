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

#include "ComRaws.h"
#include "FakeAsyncTransactionResult.h"
#include "IIqrfDb.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "ShapeProperties.h"
#include "IMessagingService.h"
#include "ITemplateService.h"
#include "ITraceService.h"
#include "ObjectFactory.h"
#include "Trace.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include <algorithm>
#include <fstream>
#include <map>

namespace iqrf {

	class JsonDpaApiRaw {
	public:
		/**
		 * Constructor
		 */
		JsonDpaApiRaw();

		/**
		 * Destructor
		 */
		virtual ~JsonDpaApiRaw();

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
		void attachInterface(IIqrfDpaService* iface);

		/**
		 * Detach DPA service interface
		 * @param iface DPA service interface
		 */
		void detachInterface(IIqrfDpaService* iface);

		/**
		 * Attach Splitter service interface
		 * @param iface Splitter service interface
		 */
		void attachInterface(IMessagingSplitterService* iface);

		/**
		 * Detach Splitter service interface
		 * @param iface Splitter service interface
		 */
		void detachInterface(IMessagingSplitterService* iface);

		/**
		 * Attach Tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService* iface);

		/**
		 * Detach Tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService* iface);

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
		 * @param msg DPA message
		 */
		void handleAsyncDpaMessage(const DpaMessage &msg);

		/// DB service interface
		IIqrfDb *m_dbService = nullptr;
		/// Splitter service interface
		IMessagingSplitterService *m_splitterService = nullptr;
		/// DPA service interface
		IIqrfDpaService *m_dpaService = nullptr;
		/// Default instance name
		std::string m_instanceName = "JsonDpaApiRaw";
		/// Indicates whether DPA message is asynchronous
		bool m_asyncDpaMessage = false;
		/// DPA message object factory
		ObjectFactory<ComNadr, rapidjson::Document&> m_objectFactory;
		/// Vector of handled DPA messages
		std::vector<std::string> m_filters = {
			"iqrfRaw",
			"iqrfRawHdp",
		};
	};
}
