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

#include "ApiMsgIqrfStandardFrc.h"
#include "IIqrfDb.h"
#include "IIqrfDpaService.h"
#include "IJsRenderService.h"
#include "IMessagingSplitterService.h"
#include "ITraceService.h"
#include "JsDriverStandardFrcSolver.h"
#include "ShapeProperties.h"
#include "Trace.h"
#include "EnumUtils.h"
#include "MessageTypes.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>

/// iqrf namespace
namespace iqrf {

	class JsonDpaApiIqrfStdExt {
	public:
		/**
		 * Constructor
		 */
		JsonDpaApiIqrfStdExt();

		/**
		 * Destructor
		 */
		virtual ~JsonDpaApiIqrfStdExt();

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
		 * @param messaging Messagin instance
		 * @param msgType Message type
		 * @param doc Request document
		 */
		void handleMsg(const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc);

		/// DB service interface
		IIqrfDb *m_dbService = nullptr;
		/// DPA service interface
		IIqrfDpaService *m_dpaService = nullptr;
		/// JS render service interface
		IJsRenderService *m_jsRenderService = nullptr;
		/// Splitter service interface
		IMessagingSplitterService *m_splitterService = nullptr;
		/// Transaction mutex
		std::mutex m_transactionMutex;
		/// Pointer to store current transaction
		std::shared_ptr<IDpaTransaction2> m_transaction;
		/// Vector of DPA messages to handle
		std::vector<std::string> m_filters = {
			"iqrfDali_Frc",
      "iqrfLight_FrcLaiRead",
      "iqrfLight_FrcLdiSend",
      "iqrfSensor_Frc"
		};
	};
}
