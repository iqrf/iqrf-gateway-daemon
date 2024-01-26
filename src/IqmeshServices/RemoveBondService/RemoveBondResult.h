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

#include "IDpaTransactionResult2.h"

#include <list>
#include <map>
#include <memory>
#include <string>

/// iqrf namespace
namespace iqrf {

	/// Node status struct
	struct NodeStatus {
		/// Node bonded
		bool bonded = false;
		/// Node successfully unbonded
		bool removed = false;
	};

	/// RemoveBond result class
	class RemoveBondResult {
	public:
		/**
		 * Get service status code
		 * @return int Service status code
		 */
		int getStatus() const {
			return m_status;
		};

		/**
		 * Set service status code
		 * @param status Service status code
		 */
		void setStatus(const int status) {
			m_status = status;
		}

		/**
		 * Get service status message
		 * @return std::string Service status message
		 */
		std::string getStatusStr() const {
			return m_statusStr;
		};

		/**
		 * Set service status message
		 * @param statusStr Service status message
		 */
		void setStatusStr(const std::string statusStr) {
			m_statusStr = statusStr;
		}

		/**
		 * Set service status code and message
		 * @param status Service status code
		 * @param statusStr Service status message
		 */
		void setStatus(const int status, const std::string statusStr) {
			m_status = status;
			m_statusStr = statusStr;
		}

		/**
		 * Get number of bonded nodes
		 * @return uint8_t Bonded nodes count
		 */
		uint8_t getNodesNr() const {
			return m_nodesNr;
		};

		/**
		 * Set number of bonded nodes
		 * @param nodesNr Bonded nodes count
		 */
		void setNodesNr(const uint8_t nodesNr) {
			m_nodesNr = nodesNr;
		}

		/**
		 * Add new node to result
		 * @param addr Node address
		 * @param bonded Node bonded
		 * @param removed Node successfully removed
		 */
		void addNodeStatus(const uint8_t &addr, bool bonded, bool removed) {
			NodeStatus status = {bonded, removed};
			m_nodes.insert(std::make_pair(addr, status));
		}

		/**
		 * Mark node bonded
		 * @param addr Node address
		 * @param bonded Node bonded
		 */
		void setNodeBonded(const uint8_t &addr, bool bonded) {
			m_nodes[addr].bonded = bonded;
		}

		/**
		 * Mark node removed
		 * @param addr Node address
		 * @param removed Node removed
		 */
		void setNodeRemoved(const uint8_t &addr, bool removed) {
			m_nodes[addr].removed = removed;
		}

		/**
		 * Get node status map
		 * @return std::map<uint8_t, NodeStatus> Node status map
		 */
		std::map<uint8_t, NodeStatus> getNodesStatus() {
			return m_nodes;
		}

		/**
		 * Store new transaction result
		 * @param transResult Transaction result
		 */
		void addTransactionResult(std::unique_ptr<IDpaTransactionResult2>& transResult) {
			m_transResults.push_back(std::move(transResult));
		}

		/**
		 * Is list of transaction results empty
		 * @return true
		 * @return false
		 */
		bool isNextTransactionResult() {
			return (m_transResults.size() > 0);
		}

		/**
		 * Pops the first transaction result
		 * @return std::unique_ptr<IDpaTransactionResult2> Transaction result
		 */
		std::unique_ptr<IDpaTransactionResult2> consumeNextTransactionResult() {
			std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = m_transResults.begin();
			std::unique_ptr<IDpaTransactionResult2> tranResult = std::move(*iter);
			m_transResults.pop_front();
			return tranResult;
		}
	private:
		// Service status
		int m_status = 0;
		// Verbose service status
		std::string m_statusStr = "ok";
		// Number of nodes in network
		uint8_t m_nodesNr = 0;
		// Map of devices
		std::map<uint8_t, NodeStatus> m_nodes;
		// List of transactions
		std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;
	};
}
