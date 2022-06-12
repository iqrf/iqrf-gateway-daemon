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

#include "rapidjson/document.h"
#include "ServiceResultBase.h"

#include <map>
#include <memory>
#include <set>

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// NetworkInfo result class
	class NetworkInfoResult : public ServiceResultBase {
	public:
		/**
		 * Gets retrieve MIDs
		 */
		bool getRetrieveMids() {
			return m_mids;
		}

		/**
		 * Sets retrieve MIDs
		 * @param mids Retrieve MIDs
		 */
		void setRetrieveMids(bool mids) {
			m_mids = mids;
		}

		/**
		 * Get retrieve VRNs
		 */
		bool getRetrieveVrns() {
			return m_vrns;
		}

		/**
		 * Sets retrieve VRNs
		 * @param vrns Retrieve VRNs
		 */
		void setRetrieveVrns(bool vrns) {
			m_vrns = vrns;
		}

		/**
		 * Get retrieve zones
		 */
		bool getRetrieveZones() {
			return m_zones;
		}

		/**
		 * Sets retrieve zones
		 * @param zones Retrieve zones
		 */
		void setRetrieveZones(bool zones) {
			m_zones = zones;
		}

		/**
		 * Get retrieve parents
		 */
		bool getRetrieveParents() {
			return m_parents;
		}

		/**
		 * Sets retrieve parents
		 * @param parents Retrieve parents
		 */
		void setRetrieveParents(bool parents) {
			m_parents = parents;
		}

		/**
		 * Sets bonded nodes
		 * @param bonded Bonded nodes
		 */
		void setBondedDevices(const std::set<uint8_t> &bonded) {
			m_bondedDevices = bonded;
		}

		/**
		 * Sets discovered nodes
		 * @param discovered Discovered nodes
		 */
		void setDiscoveredDevices(const std::set<uint8_t> &discovered) {
			m_discoveredDevices = discovered;
		}

		/**
		 * Sets node mids
		 * @param mids Map of node addresses and mids
		 */
		void setMids(const std::map<uint8_t, uint32_t> &mids) {
			m_midMap = mids;
		}

		/**
		 * Sets node vrns
		 * @param vrns Map of node addresses and vrns
		 */
		void setVrns(const std::map<uint8_t, uint8_t> &vrns) {
			m_vrnMap = vrns;
		}

		/**
		 * Sets node zones
		 * @param zones Map of node addresses and zones
		 */
		void setZones(const std::map<uint8_t, uint8_t> &zones) {
			m_zoneMap = zones;
		}

		/**
		 * Sets node parents
		 * @param parents Map of node addresses and parents
		 */
		void setParents(const std::map<uint8_t, uint8_t> &parents) {
			m_parentMap = parents;
		}

		/**
		 * Populates response document
		 * @param response Response document
		 */
		void createResponse(Document &response) {
			// Default parameters
			ServiceResultBase::setResponseMetadata(response);

			// Service results
			if (m_status == 0) {
				Value array(kArrayType);
				Document::AllocatorType &allocator = response.GetAllocator();
				for (auto &addr : m_bondedDevices) {
					Value object(kObjectType);
					Pointer("/address").Set(object, addr, allocator);
					bool discovered = m_discoveredDevices.find(addr) != m_discoveredDevices.end();
					if (m_mids) {
						Pointer("/mid").Set(object, m_midMap[addr], allocator);				
					}
					if (m_vrns) {
						Pointer("/vrn").Set(object, discovered ? m_vrnMap[addr] : 0, allocator);
					}
					if (m_zones) {
						Pointer("/zone").Set(object, discovered ? m_zoneMap[addr]: 0, allocator);
					}
					if (m_parents) {
						if (discovered) {
							Pointer("/parent").Set(object, m_parentMap[addr], allocator);
						} else {
							Pointer("/parent").Create(object, allocator);
						}
					}
					array.PushBack(object, allocator);
				}
				Pointer("/data/rsp/nodes").Set(response, array);
			}

			// Transactions and error codes
			ServiceResultBase::createResponse(response);
		}
	private:
		/// Get MIDs
		bool m_mids;
		/// Get VRNs
		bool m_vrns;
		/// Get Zones
		bool m_zones;
		/// Get Parents
		bool m_parents;
		/// Set of bonded nodes
		std::set<uint8_t> m_bondedDevices;
		/// Set of discovered nodes
		std::set<uint8_t> m_discoveredDevices;
		/// Map of node addresses and mids
		std::map<uint8_t, uint32_t> m_midMap;
		/// Map of node addresses and vrns
		std::map<uint8_t, uint8_t> m_vrnMap;
		/// Map of node addresses and zones
		std::map<uint8_t, uint8_t> m_zoneMap;
		/// Map of node addresses and parents
		std::map<uint8_t, uint8_t> m_parentMap;
	};
}
