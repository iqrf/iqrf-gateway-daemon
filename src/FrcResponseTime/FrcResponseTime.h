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

#include "ComIqmeshMaintenanceFrcResponse.h"
#include "FrcResponseTimeResult.h"
#include "IFrcResponseTime.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"

#include "Exceptions/NoBondedNodesException.h"
#include "Exceptions/NoRespondedNodesException.h"

#include "ShapeProperties.h"

#include <cmath>

#define FRC_MAX_NODES 63
#define FRC_RESPONSE_MAX_BYTES 55
#define FRC_EXTRA_RESPONSE_BYTES 9

/// iqrf namespace
namespace iqrf {

	class FrcResponseTime : public IFrcResponseTime {
	public:
		/// Service error codes
		enum ErrorCodes {
			serviceError = 1000,
			requestParseError = 1001,
			exclusiveAccessError = 1002,
			noBondedNodesError = 1003,
			noRespondedNodesError = 1004,

		};

		/**
		 * Constructor
		 */
		FrcResponseTime();

		/**
		 * Destructor
		 */
		virtual ~FrcResponseTime();

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
		 * Attaches DPA service interface
		 * @param iface DPA service interface
		 */
		void attachInterface(iqrf::IIqrfDpaService *iface);

		/**
		 * Detaches DPA service interface
		 * @param iface DPA service interface
		 */
		void detachInterface(iqrf::IIqrfDpaService *iface);

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
		 * Converts node bitmap to set
		 * @param bitmap Node bitmap
		 * @return Set of node addresses
		 */
		std::set<uint8_t> nodeBitmapToSet(const uint8_t *bitmap);

		/**
		 * Converts set of nodes to FRC selected nodes bitmap
		 * @param nodes Node address set
		 * @param idx Index of node address in set to start from
		 * @param count Number of node addresses to insert to bitmap
		 * @return Node address bitmap
		 */
		std::vector<uint8_t> selectNodes(const std::set<uint8_t> &nodes, const uint8_t &idx, const uint8_t &count);

		/**
		 * Set transaction result, error code and error string
		 * @param serviceResult Service result
		 * @param result Transaction result
		 * @param errorStr Error string
		 */
		void setErrorTransactionResult(FrcResponseTimeResult &serviceResult, std::unique_ptr<IDpaTransactionResult2> &result, const std::string &errorStr);

		/**
		 * Retrieves bonded nodes from the network
		 * @param serviceResult Service result
		 */
		void getBondedNodes(FrcResponseTimeResult &serviceResult);

		/**
		 * Sets FRC response time
		 * @param serviceResult Service result
		 * @param FrcResponseTime FRC response time
		 * @return Previous FRC response time
		 */
		IDpaTransaction2::FrcResponseTime setFrcResponseTime(FrcResponseTimeResult &serviceResult, IDpaTransaction2::FrcResponseTime responseTime);

		/**
		 * Determines best FRC response time
		 * @param serviceResult Service result
		 * @return Slowest response time from network
		 */
		IDpaTransaction2::FrcResponseTime getResponseTime(FrcResponseTimeResult &serviceResult);

		/**
		 * Sends selective FRC send request
		 * @param serviceResult Service result
		 * @param count Number of selected nodes
		 * @param processed Number of total processed nodes
		 * @param responded Number of total responded nodes
		 * @param data FRC response data container
		 */
		void frcSendSelective(FrcResponseTimeResult &serviceResult, const uint8_t &count, const uint8_t &processed, uint8_t &responded, std::vector<uint8_t> &data);

		/**
		 * Sends FRC extra result request
		 * @param serviceResult Service result
		 * @param count Number of nodes to extract from extra result
		 * @param data FRC response data container
		 */
		void frcExtraResult(FrcResponseTimeResult &serviceResult, const uint8_t &count, std::vector<uint8_t> &data);

		/**
		 * Handles request from splitter
		 * @param messagingId Messaging ID
		 * @param msgType Message type
		 * @param doc request document
		 */
		void handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc);

		/// Message type
		const std::vector<std::string> m_mTypes = {
			"iqmeshNetwork_MaintenanceFrcResponseTime"
		};
		/// Request parameters
		TFrcResponseTimeParams m_requestParams;
		/// DPA service
		IIqrfDpaService *m_dpaService = nullptr;
		/// Splitter service
		IMessagingSplitterService *m_splitterService = nullptr;
		/// Exclusive access
		std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
	};
}
