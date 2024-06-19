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
#include "ServiceResultBase.h"

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	class DeviceData {
		public:
			uint16_t hwpid = 0;
			uint32_t mid = 0;
			uint8_t rssi = 0;
			std::vector<sensor::item::Sensor> sensors;
	};

	/// Sensor data result class
	class SensorDataResult : public ServiceResultBase {
	public:
		/**
		 * Stores device HWPID
		 * @param address Device address
		 * @param hwpid Device HWPID
		 */
		void setDeviceHwpid(const uint8_t &address, const uint16_t &hwpid) {
			if (!m_deviceData.count(address)) {
				DeviceData data;
				data.hwpid = hwpid;
				m_deviceData.emplace(address, data);
				return;
			}
			m_deviceData[address].hwpid = hwpid;
		}

		/**
		 * Stores device MID
		 * @param address Device address
		 * @param mid Device MID
		 */
		void setDeviceMid(const uint8_t &address, const uint32_t &mid) {
			if (!m_deviceData.count(address)) {
				DeviceData data;
				data.mid = mid;
				m_deviceData.emplace(address, data);
				return;
			}
			m_deviceData[address].mid = mid;
		}

		/**
		 * Stores device RSSI
		 * @param address Device address
		 * @param rssi Device RSSI
		 */
		void setDeviceRssi(const uint8_t &address, const uint8_t &rssi) {
			if (rssi == 0) {
				return;
			}
			if (!m_deviceData.count(address)) {
				DeviceData data;
				data.rssi = rssi;
				m_deviceData.emplace(address, data);
				return;
			}
			m_deviceData[address].rssi = rssi;
		}

		std::set<uint8_t> getNodesWithoutRssi() {
			std::set<uint8_t> nodes;
			for (auto &[addr, metadata] : m_deviceData) {
				if (metadata.rssi == 0) {
					nodes.insert(addr);
				}
			}
			return nodes;
		}

		/**
		 * Stores collected sensor data
		 * @param sensorData Collected sensor data
		 */
		void addSensorData(const std::vector<sensor::item::Sensor> &sensorData) {
			for (const auto& sensor : sensorData) {
				const auto &addr = sensor.getAddr();
				if (!m_deviceData.count(addr)) {
					DeviceData data;
					data.sensors.push_back(sensor);
					m_deviceData.emplace(addr, data);
					return;
				}
				m_deviceData[addr].sensors.emplace_back(sensor);
			}
		}

		/**
		 * Returns map of devices and sensor data
		 * @return Devices and sensor data
		 */
		std::map<uint8_t, std::vector<sensor::item::Sensor>> getSensorData() {
			std::map<uint8_t, std::vector<sensor::item::Sensor>> sensorData;
			for (auto &[addr, device] : m_deviceData) {
				sensorData[addr] = device.sensors;
			}
			return sensorData;
		}

		void createStartMessage(Document &doc) {
			ServiceResultBase::setResponseMetadata(doc);
			Pointer("/data/rsp/reading").Set(doc, true);
			ServiceResultBase::createResponse(doc);
		}

		/**
		 * Populates response document
		 * @param response Response document
		 */
		void createResultMessage(Document &doc) {
			// Default parameters
			ServiceResultBase::setResponseMetadata(doc);

			// Service results
			if (m_status == 0) {
				Value array(kArrayType);
				Document::AllocatorType &allocator = doc.GetAllocator();
				// devices
				for (auto &[addr, data] : m_deviceData) {
					Value device(kObjectType);
					Pointer("/address").Set(device, addr, allocator);

					Pointer("/hwpid").Set(device, data.hwpid, allocator);
					if (data.mid != 0) {
						Pointer("/mid").Set(device, data.mid, allocator);
					} else {
						Pointer("/mid").Set(device, rapidjson::Value(kNullType), allocator);
					}
					if (data.rssi != 0) {
						Pointer("/rssi").Set(device, data.rssi - 130, allocator);
					} else {
						Pointer("/rssi").Set(device, rapidjson::Value(kNullType), allocator);
					}

					std::vector<sensor::item::Sensor> &sensors = data.sensors;
					std::sort(sensors.begin(), sensors.end(), [](sensor::item::Sensor a, sensor::item::Sensor b) {
						return a.getIdx() < b.getIdx();
					});
					Value sensorArray(kArrayType);
					// sensors
					for (auto &sensorItem : sensors) {
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
				Pointer("/data/rsp/devices").Set(doc, array, allocator);
				Pointer("/data/rsp/reading").Set(doc, false);
			}

			// Transactions and error codes
			ServiceResultBase::createResponse(doc);
		}
	private:
		/// Device metadata
		std::map<uint8_t, DeviceData> m_deviceData;
	};
}
