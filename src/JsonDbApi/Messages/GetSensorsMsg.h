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

#include "BaseMsg.h"

using namespace rapidjson;

namespace iqrf {

	/**
	 * Get sensors request message
	 */
	class GetSensorsMsg : public BaseMsg {
	public:
		/// Delete base constructor
		GetSensorsMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetSensorsMsg(const Document &doc) : BaseMsg(doc) {};

		/**
		 * Handles get sensors request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			sensors = dbService->getDeviceAddressSensorMap();
		}

		/**
		 * Populates response document with sensors response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			if (m_status == 0) {
				Value array(kArrayType);
				Document::AllocatorType &allocator = doc.GetAllocator();

				for (auto &[address, sensorVector] : sensors) {
					Value deviceObject;
					Pointer("/address").Set(deviceObject, address, allocator);

					Value sensorArray(kArrayType);

					for (auto &[deviceSensor, sensor] : sensorVector) {
						Value sensorObject;
						Pointer("/index").Set(sensorObject, deviceSensor.getGlobalIndex(), allocator);
						Pointer("/type").Set(sensorObject, sensor.getType(), allocator);
						Pointer("/name").Set(sensorObject, sensor.getName(), allocator);
						Pointer("/shortname").Set(sensorObject, sensor.getShortname(), allocator);
						Pointer("/unit").Set(sensorObject, sensor.getUnit(), allocator);
						Pointer("/decimalPlaces").Set(sensorObject, sensor.getDecimals(), allocator);
						Pointer("/frc2Bit").Set(sensorObject, sensor.hasFrc2Bit(), allocator);
						Pointer("/frc1Byte").Set(sensorObject, sensor.hasFrcByte(), allocator);
						Pointer("/frc2Byte").Set(sensorObject, sensor.hasFrc2Byte(), allocator);
						Pointer("/frc4Byte").Set(sensorObject, sensor.hasFrc4Byte(), allocator);
						std::shared_ptr<std::string> metadata = deviceSensor.getMetadata();
						Document metadataDoc;
						if (metadata) {
							metadataDoc.Parse(metadata->c_str());
						}
						if (sensor.getType() == 192) {
							if (metadata && metadataDoc.HasMember("datablock")) {
								Pointer("/value").Set(sensorObject, Value(metadataDoc["datablock"], allocator).Move(), allocator);
								metadataDoc.RemoveMember("datablock");
							} else {
								Pointer("/value").Create(sensorObject, allocator);
							}
						} else {
							if (deviceSensor.getValue().has_value()) {
								Pointer("/value").Set(sensorObject, deviceSensor.getValue().value(), allocator);
							} else {
								Pointer("/value").Create(sensorObject, allocator);
							}
						}
						std::shared_ptr<std::string> updated = deviceSensor.getUpdated();
						if (updated) {
							Pointer("/updated").Set(sensorObject, *updated.get(), allocator);
						} else {
							Pointer("/updated").Create(sensorObject, allocator);
						}
						if (metadata) {
							Pointer("/metadata").Set(sensorObject, Value(metadataDoc, allocator).Move(), allocator);
						} else {
							Pointer("/metadata").Create(sensorObject, allocator);
						}
						sensorArray.PushBack(sensorObject, allocator);
					}

					Pointer("/sensors").Set(deviceObject, sensorArray, allocator);

					array.PushBack(deviceObject, allocator);
				}

				Pointer("/data/rsp/sensorDevices").Set(doc, array, allocator);
			}
			BaseMsg::createResponsePayload(doc);
		}
	private:
		/// Map of sensor device tuples
		std::map<uint8_t, std::vector<std::pair<DeviceSensor, Sensor>>> sensors;
	};
}
