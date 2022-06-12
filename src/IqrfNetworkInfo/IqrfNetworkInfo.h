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

#include "IIqrfDpaService.h"
#include "IIqrfNetworkInfo.h"
#include "ITraceService.h"
#include "ShapeProperties.h"

#include <mutex>

#define EEEPROM_READ_MAX_LEN 54
#define MID_START_ADDR 0x4000
#define VRN_START_ADDR 0x5000
#define ZONE_START_ADDR 0x5200
#define PARENT_START_ADDR 0x5300

/// iqrf namespace
namespace iqrf {
	/// IqrfNetworkInfo class
	class IqrfNetworkInfo : public IIqrfNetworkInfo {
	public:
		/**
		 * Constructor
		 */
		IqrfNetworkInfo();

		/**
		 * Destructor
		 */
		virtual ~IqrfNetworkInfo();

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
		void attachInterface(IIqrfDpaService *iface);

		/**
		 * Detaches DPA service interface
		 * @param iface DPA service interface
		 */
		void detachInterface(IIqrfDpaService *iface);

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

		void getNetworkInfo(NetworkInfoResult &result, const uint8_t &repeat) override;

		std::list<std::unique_ptr<IDpaTransactionResult2>> getTransactionResults() override;

		int getErrorCode() override;
	private:
		/**
		 * Handles transaction error
		 * @param result Transaction result
		 * @param errorStr Exception message
		 */
		void handleTransactionErrors(std::unique_ptr<IDpaTransactionResult2> &result, const std::string &errorStr);

		/**
		 * Retrieves bonded nodes
		 * @param repeat Transaction repeats
		 * @return Set of device addresses
		 */
		std::set<uint8_t> getBondedDevices(const uint8_t &repeat);

		/**
		 * Retrieves discovered nodes
		 * @param nodes Network devices
		 * @param repeat Transaction repeats
		 * @return Set of discovered device addresses
		 */
		std::set<uint8_t> getDiscoveredDevices(const std::set<uint8_t> &nodes, const uint8_t &repeat);

		/**
		 * Retrieves MIDs of network devices
		 * @param nodes Network devices
		 * @param repeat Transaction repeats
		 * @return Map of node addresses and MIDs
		 */
		std::map<uint8_t, uint32_t> getMids(const std::set<uint8_t> &nodes, const uint8_t &repeat);

		/**
		 * Retrieves VRNs of network devices
		 * @param nodes Network devices
		 * @param repeat Transaction repeats
		 * @return Map of node addresses and VRNs
		 */
		std::map<uint8_t, uint8_t> getVrns(const std::set<uint8_t> &nodes, const uint8_t &repeat);

		std::map<uint8_t, uint8_t> getZones(const std::set<uint8_t> &nodes, const uint8_t &repeat);

		std::map<uint8_t, uint8_t> getParents(const std::set<uint8_t> &nodes, const uint8_t &repeat);

		void eeepromRead(std::vector<uint8_t> &data, const uint16_t &address, const uint8_t &length, const uint8_t &repeat);

		/// DPA service
		IIqrfDpaService *m_dpaService = nullptr;
		/// Exclusive access
		std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
		/// Transaction results
		std::list<std::unique_ptr<IDpaTransactionResult2>> m_transactionResults;
		/// Network info mutex
		std::mutex m_mtx;
		/// Error code
		int m_errorCode;
	};
}
