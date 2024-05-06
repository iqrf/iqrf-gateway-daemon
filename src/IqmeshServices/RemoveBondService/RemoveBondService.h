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

#include "ComIqmeshNetworkRemoveBond.h"
#include "IIqrfNetworkEnum.h"
#include "IIqrfDpaService.h"
#include "IJsCacheService.h"
#include "IMessagingSplitterService.h"
#include "IRemoveBondService.h"
#include "ITraceService.h"
#include "RemoveBondResult.h"
#include "ShapeProperties.h"
#include "Trace.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <list>
#include <map>
#include <string>
#include <thread>

namespace iqrf {

	/// Remove bond service class
	class RemoveBondService : public IRemoveBondService {
	public:
		/// Service error codes
		enum ErrorCodes {
			serviceError = 1000,
			requestParseError = 1001,
			exclusiveAccessError = 1002,
			noDeviceError = 1003,
			partailFailureError = 1004,
		};

		/**
		 * Constructor
		 */
		RemoveBondService();

		/**
		 * Destructor
		 */
		virtual ~RemoveBondService();

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
		void attachInterface(iqrf::IIqrfDpaService* iface);

		/**
		 * Detaches DPA service interface
		 * @param iface DPA service interface
		 */
		void detachInterface(iqrf::IIqrfDpaService* iface);

		/**
		 * Attaches network enum service interface
		 * @param iface Network enum interface
		 */
		void attachInterface(iqrf::IIqrfNetworkEnum *iface);

		/**
		 * Detaches DB service interface
		 * @param iface DB service interface
		 */
    void detachInterface(iqrf::IIqrfNetworkEnum *iface);

		/**
		 * Attaches splitter service interface
		 * @param iface Splitter service interface
		 */
		void attachInterface(iqrf::IMessagingSplitterService* iface);

		/**
		 * Detaches splitter service interface
		 * @param iface Splitter service interface
		 */
		void detachInterface(iqrf::IMessagingSplitterService* iface);

		/**
		 * Attaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService* iface);

		/**
		 * Detaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService* iface);

		/**
		 * Handles request from splitter
		 * @param messagingId Messaging ID
		 * @param msgType Message type
		 * @param doc request document
		 */
		void handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc);

	private:
		/**
		 * Get set of bonded nodes
		 * @param removeBondResult Service result
		 * @return std::set<uint8_t> Bonded nodes addreses
		 */
		std::set<uint8_t> getBondedNodes(RemoveBondResult& removeBondResult);

		/**
		 * Get coordinator addressing information
		 * @param removeBondResult Service result
		 * @return TPerCoordinatorAddrInfo_Response Addressing information
		 */
		TPerCoordinatorAddrInfo_Response getAddressingInfo(RemoveBondResult& removeBondResult);

		/**
		 * Set FRC response time and get previously configured value
		 * @param removeBondResult Service result
		 * @param FRCresponseTime FRC response time value
		 * @return uint8_t Previous FRC response time value
		 */
		uint8_t setFrcReponseTime(RemoveBondResult& removeBondResult, uint8_t FRCresponseTime);

		/**
		 * Remove bonds at [N] using FRC acknowledgement
		 * @param removeBondResult Service result
		 * @param PNUM Peripheral
		 * @param PCMD Peripheral command
		 * @param hwpId HWPID
		 * @param nodes Node addresses to remove
		 * @return std::set<uint8_t> Set of nodes that acknowledged remove bond command
		 */
		std::set<uint8_t> FRCAcknowledgedBroadcastBits(RemoveBondResult& removeBondResult, const uint8_t PNUM, const uint8_t PCMD, const uint16_t hwpId, const std::set<uint8_t> &nodes);

		/**
		 * Remove bond at [C]
		 * @param removeBondResult Service result
		 * @param nodeAddr Node address to remove
		 */
		void coordRemoveBond(RemoveBondResult& removeBondResult, const uint8_t nodeAddr);

		/**
		 * Remove multiple bonds at [C]
		 * @param removeBondResult Service result
		 * @param nodes Node addresses to remove
		 */
		void coordRemoveBondBatch(RemoveBondResult& removeBondResult, std::set<uint8_t> &nodes);

		/**
		 * Clear all bonds at [C]
		 * @param removeBondResult Service result
		 */
		void clearAllBonds(RemoveBondResult& removeBondResult);

		/**
		 * Remove multiple bonds at [N]
		 * @param removeBondResult Service result
		 * @param nodeAddr Node address to remove
		 * @param hwpId HWPID filter
		 */
		void nodeRemoveBond(RemoveBondResult& removeBondResult, const uint8_t nodeAddr, const uint16_t hwpId);

		/**
		 * Handle remove bond request
		 * @param removeBondResult Service params and result
		 */
		void removeBond(RemoveBondResult& removeBondResult);

		/**
		 * Handle remove bond in coordinator request
		 * @param removeBondResult Service params and result
		 */
		void removeBondOnlyInC(RemoveBondResult& removeBondResult);

		/**
		 * Invoke IQRF DB Enumeration
		 */
		void invokeDbEnumeration();

		/**
		 * Create and send response from status
		 * @param status Service status code
		 * @param statusStr Service status message
		 */
		void createResponse(const int status, const std::string &statusStr);

		/**
		 * Create and send response from service result
		 * @param removeBondResult Service result
		 */
		void createResponse(RemoveBondResult& removeBondResult);

		/// Remove bond message type
		const std::string m_mTypeName_iqmeshNetworkRemoveBond = "iqmeshNetwork_RemoveBond";
		/// Service request parameters
		TRemoveBondRequestParams m_requestParams;
		/// DB service
		IIqrfNetworkEnum *m_networkEnumService = nullptr;
		/// DPA service
		IIqrfDpaService *m_dpaService = nullptr;
		/// Splitter service
		IMessagingSplitterService *m_splitterService = nullptr;
		/// Exclusive access
		std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
		/// Messaging ID
		const std::string* m_messagingId = nullptr;
		/// Message type
		const IMessagingSplitterService::MsgType* m_msgType = nullptr;
		/// Remove bond request document
		const ComIqmeshNetworkRemoveBond* m_comRemoveBond = nullptr;
		/// Coordinator remove bond request timeout in milliseconds
		const uint8_t m_coordinatorRemoveBondTimeout = 15;
	};
}
