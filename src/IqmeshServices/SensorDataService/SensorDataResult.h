/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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

#include "JsDriverSensor.h"
#include "rapidjson/document.h"
#include "SensorDataParams.h"
#include "ServiceResultBase.h"

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// Sensor data result class
	class SensorDataResult : public ServiceResultBase {
	public:
		/**
		 * Stores device HWPID
		 * @param address Device address
		 * @param hwpid Device HWPID
		 */
		void addDeviceHwpid(const uint8_t &address, const uint16_t &hwpid) {
			m_hwpid.insert(std::make_pair(address, hwpid));
		}

		/**
		 * Stores device MID
		 * @param address Device address
		 * @param mid Device MID
		 */
		void addDeviceMid(const uint8_t &address, const uint32_t &mid) {
			m_mids.insert(std::make_pair(address, mid));
		}

		/**
		 * Stores collected sensor data
		 * @param sensorData Collected sensor data
		 */
		void addSensorData(const std::vector<sensor::item::SensorPtr> &sensorData) {
			for (const auto& sensor : sensorData) {
				m_sensorData[sensor->getAddr()].emplace_back(*sensor.get());
			}
		}

		void setRssi(const std::map<uint8_t, uint8_t> &rssiMap) {
			m_rssi = rssiMap;
		}

		/**
		 * Returns map of devices and sensor data
		 * @return Devices and sensor data
		 */
		std::map<uint8_t, std::vector<sensor::item::Sensor>> getSensorData() {
			return m_sensorData;
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
				// devices
				for (auto &deviceItem : m_sensorData) {
					Value device(kObjectType);
					Pointer("/address").Set(device, deviceItem.first, allocator);
					auto hwpid = m_hwpid[deviceItem.first];
					Pointer("/hwpid").Set(device, hwpid, allocator);
					auto mid = m_mids[deviceItem.first];
					if (mid != 0) {
						Pointer("/mid").Set(device, mid, allocator);
					} else {
						Pointer("/mid").Set(device, rapidjson::Value(kNullType), allocator);
					}
					if (m_rssi.find(deviceItem.first) != m_rssi.end()) {
						Pointer("/rssi").Set(device, m_rssi[deviceItem.first] - 130, allocator);
					} else {
						Pointer("/rssi").Set(device, rapidjson::Value(kNullType), allocator);
					}
					Value sensorArray(kArrayType);
					// sensors
					for (auto &sensorItem : deviceItem.second) {
						Value sensor(kObjectType);
						Pointer("/index").Set(sensor, sensorItem.getIdx(), allocator);
						Pointer("/type").Set(sensor, sensorItem.getType(), allocator);
						Pointer("/name").Set(sensor, sensorItem.hasBreakdown() ? sensorItem.getBreakdownName() : sensorItem.getName(), allocator);
						Pointer("/unit").Set(sensor, sensorItem.hasBreakdown() ? sensorItem.getBreakdownUnit() : sensorItem.getUnit(), allocator);
						if (sensorItem.isValueSet()) {
							if (sensorItem.getType() != 192) {
								Pointer("/value").Set(sensor, sensorItem.hasBreakdown() ? sensorItem.getBreakdownValue() : sensorItem.getValue(), allocator);
							} else {
								auto vals = sensorItem.hasBreakdown() ?  sensorItem.getBreakdownValueArray() : sensorItem.getValueArray();
								Value datablock(kArrayType);
								for (auto &val : vals) {
									datablock.PushBack(Value(val), allocator);
								}
								Pointer("/value").Set(sensor, datablock, allocator);
							}
						} else {
							Pointer("/value").Set(sensor, rapidjson::Value(kNullType), allocator);
						}
						sensorArray.PushBack(sensor, allocator);
					}
					Pointer("/sensors").Set(device, sensorArray, allocator);
					array.PushBack(device, allocator);
				}
				Pointer("/data/rsp/devices").Set(response, array, allocator);
			}

			// Transactions and error codes
			ServiceResultBase::createResponse(response);
		}
	private:
		/// Device HWPIDs
		std::map<uint8_t, uint16_t> m_hwpid;
		/// Device MIDs
		std::map<uint8_t, uint32_t> m_mids;
		/// Device RSSI
		std::map<uint8_t, uint8_t> m_rssi;
		/// Device sensors data
		std::map<uint8_t, std::vector<sensor::item::Sensor>> m_sensorData;
	};
}
