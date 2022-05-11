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

#include "IMessagingSplitterService.h"
#include "rapidjson/document.h"
#include "ServiceResultBase.h"

#include <memory>
#include <set>

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// FRC Response Time result class
	class FrcResponseTimeResult : public ServiceResultBase {
	public:
		/**
		 * Returns set of bonded nodes
		 * @return Bonded nodes
		 */
		const std::set<uint8_t> &getBondedNodes() const {
			return m_bondedNodes;
		}

		/**
		 * Stores set of bonded nodes
		 * @param nodes Bonded nodes
		 */
		void setBondedNodes(const std::set<uint8_t> &nodes) {
			m_bondedNodes = nodes;
		}

		/**
		 * Sets number of inaccessible nodes
		 * @param responded Number of nodes that responded
		 */
		void setInaccessibleNodes(const uint8_t &responded) {
			m_inaccessibleNodes = (m_bondedNodes.size() - responded);
		}

		/**
		 * Sets number of nodes ignoring the event
		 * @param unhandled Number of nodes ignoring the event
		 */
		void setUnhandledNodes(const uint8_t &unhandled) {
			m_unhandledNodes = unhandled;
		}

		/**
		 * Sets map of nodes and their response times
		 * @param map Map of nodes and response times
		 */
		void setResponseTimeMap(const std::map<uint8_t, uint8_t> &map) {
			m_responseTimeMap = map;
		}

		/**
		 * Returns current response time
		 * @return Current response time
		 */
		const IDpaTransaction2::FrcResponseTime &getCurrentResponseTime() const {
			return m_currentResponseTime;
		}

		/**
		 * Sets current response time
		 * @param responseTime Current response time
		 */
		void setCurrentResponseTime(const IDpaTransaction2::FrcResponseTime &responseTime) {
			m_currentResponseTime = responseTime;
		}

		/**
		 * Returns recommended response time
		 * @return Recommended response time
		 */
		const IDpaTransaction2::FrcResponseTime &getRecommendedResponseTime() const {
			return m_recommendedResponseTime;
		}

		/**
		 * Sets recommended response time
		 * @param responseTime Recommended response time
		 */
		void setRecommendedResponseTime(const IDpaTransaction2::FrcResponseTime &responseTime) {
			m_recommendedResponseTime = responseTime;
		}

		/**
		 * Converts FRC response time to milliseconds
		 * @param responseTime Response time
		 * @return Response time in milliseconds
		 */
		uint16_t responseTimeToMs(const IDpaTransaction2::FrcResponseTime &responseTime) {
			uint16_t ms = 40;
			switch (responseTime) {
			case IDpaTransaction2::FrcResponseTime::k360Ms:
				ms = 360;
				break;
			case IDpaTransaction2::FrcResponseTime::k680Ms:
				ms = 680;
				break;
			case IDpaTransaction2::FrcResponseTime::k1320Ms:
				ms = 1320;
				break;
			case IDpaTransaction2::FrcResponseTime::k2600Ms:
				ms =  2600;
				break;
			case IDpaTransaction2::FrcResponseTime::k5160Ms:
				ms = 5160;
				break;
			case IDpaTransaction2::FrcResponseTime::k10280Ms:
				ms = 10280;
				break;
			case IDpaTransaction2::FrcResponseTime::k20620Ms:
				ms = 20620;
				break;
			default:
				ms = 40;
			}
			return ms;
		}

		/**
		 * Populates response document
		 * @param response response document
		 */
		void createResponse(Document &response) {
			// Default parameters
			ServiceResultBase::setResponseMetadata(response);

			// Service results
			if (m_status == 0) {
				// Inacessible nodes
				Pointer("/data/rsp/inaccessibleNodes").Set(response, m_inaccessibleNodes);
				// Nodes ignored
				Pointer("/data/rsp/unhandledNodes").Set(response, m_unhandledNodes);

				// Node results
				Value array(kArrayType);
				Document::AllocatorType &allocator = response.GetAllocator();
				for (auto &item : m_responseTimeMap) {
					uint8_t address = item.first;
					uint8_t responseTime = item.second;

					Value object(kObjectType);
					Pointer("/deviceAddr").Set(object, address, allocator);
					Pointer("/responded").Set(object, responseTime != 0, allocator);
					if (responseTime != 0) {
						Pointer("/handled").Set(object, responseTime != 0xFF, allocator);
						if (responseTime != 0xFF) {
							Pointer("/responseTime").Set(object, responseTimeToMs((IDpaTransaction2::FrcResponseTime)(responseTime - 1)), allocator);
						}
					}
					array.PushBack(object, allocator);
				}
				Pointer("/data/rsp/nodes").Set(response, array);

				// FRC response time results
				Pointer("/data/rsp/currentResponseTime").Set(response, responseTimeToMs(m_currentResponseTime));
				Pointer("/data/rsp/recommendedResponseTime").Set(response, responseTimeToMs(m_recommendedResponseTime));
			}

			// Transactions and error codes
			ServiceResultBase::createResponse(response);
		}
	private:
		/// Set of bonded node addresses
		std::set<uint8_t> m_bondedNodes;
		/// Number of inaccessible nodes
		uint8_t m_inaccessibleNodes = 0;
		/// Number of nodes ignoring the event
		uint8_t m_unhandledNodes = 0;
		/// Map of node addresses and response times
		std::map<uint8_t, uint8_t> m_responseTimeMap;
		/// Current FRC response time
		IDpaTransaction2::FrcResponseTime m_currentResponseTime = IDpaTransaction2::FrcResponseTime::k40Ms;
		/// Recommended FRC response time
		IDpaTransaction2::FrcResponseTime m_recommendedResponseTime = IDpaTransaction2::FrcResponseTime::k40Ms;
	};
}
