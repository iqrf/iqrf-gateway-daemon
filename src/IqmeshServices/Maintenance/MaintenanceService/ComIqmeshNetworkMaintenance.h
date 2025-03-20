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

#include "ComBase.h"
#include "IDpaTransaction2.h"
#include "JsonUtils.h"
#include "Trace.h"

#include <list>

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// Maintenance input paramaters
	typedef struct {
		uint16_t deviceAddr = COORDINATOR_ADDRESS;
		uint8_t rfChannel = 0;
		uint8_t rxFilter = 0;
		IDpaTransaction2::FrcResponseTime measurementTime = IDpaTransaction2::FrcResponseTime::k40Ms;
		int measurementTimeMS = 40;
		int repeat = 1;
	} TMaintenanceInputParams;

	class ComIqmeshNetworkMaintenance : public ComBase {
	public:
		/**
		 * Base constructor
		 */
		ComIqmeshNetworkMaintenance() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		explicit ComIqmeshNetworkMaintenance(Document &doc) : ComBase(doc) {
			parse(doc);
		}

		/**
		 * Destructor
		 */
		virtual ~ComIqmeshNetworkMaintenance() {}

		/**
		 * Returns maintenance request parameters
		 * @return const TMaintenanceInputParams
		 */
		const TMaintenanceInputParams getMaintenanceInputParams() const {
			return m_MaintenanceParams;
		}
	protected:
		/**
		 * Populates response document
		 * @param doc Response document
		 * @param res Transaction result
		 */
		void createResponsePayload(Document &doc, const IDpaTransactionResult2 &res) override {
			Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
		}

	private:
		/**
		 * Parses request document and stores maintenance request parameters
		 * @param doc Request document
		 */
		void parse(Document &doc) {
			Value *jsonVal;
			// Repeat
			if ((jsonVal = Pointer("/data/repeat").Get(doc))) {
				m_MaintenanceParams.repeat = jsonVal->GetInt();
			}
			// deviceAddr
			if ((jsonVal = Pointer("/data/req/deviceAddr").Get(doc))) {
				m_MaintenanceParams.deviceAddr = (uint16_t)jsonVal->GetUint();
			}
			// RFchannel
			if ((jsonVal = Pointer("/data/req/rfChannel").Get(doc))) {
				m_MaintenanceParams.rfChannel = (uint8_t)jsonVal->GetUint();
				if (m_MaintenanceParams.rfChannel > 67) {
					m_MaintenanceParams.rfChannel = 0;
				}
			}
			// RFfilter
			if ((jsonVal = Pointer("/data/req/rxFilter").Get(doc))) {
				m_MaintenanceParams.rxFilter = (uint8_t)jsonVal->GetUint();
				if (m_MaintenanceParams.rxFilter > 64 && m_MaintenanceParams.rxFilter != 255) {
					m_MaintenanceParams.rxFilter = 0;
				}
			}
			// measurementTime
			if ((jsonVal = Pointer("/data/req/measurementTime").Get(doc))) {
				m_MaintenanceParams.measurementTimeMS = jsonVal->GetUint();
				switch (m_MaintenanceParams.measurementTimeMS) {
					case 360:
						m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k360Ms;
						break;
					case 680:
						m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k680Ms;
						break;
					case 1320:
						m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k1320Ms;
						break;
					case 2600:
						m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k2600Ms;
						break;
					case 5160:
						m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k5160Ms;
						break;
					case 10280:
						m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k10280Ms;
						break;
					case 20620:
						m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k20620Ms;
						break;
					default:
						m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k40Ms;
						break;
				}
			}
		}

		/// maintenance request parameters
		TMaintenanceInputParams m_MaintenanceParams;
	};
}
