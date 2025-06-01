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
	class LegacyGetSensorsMsg : public BaseMsg {
	public:
		/// Delete base constructor
		LegacyGetSensorsMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		LegacyGetSensorsMsg(const Document &doc) : BaseMsg(doc) {};

		/**
		 * Handles get sensors request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			includeMetadata = dbService->getMetadataToMessages();
			devSenMap = dbService->getDeviceAddressSensorMap();
			for (const auto &[address, sensors] : devSenMap) {
				metadata.insert_or_assign(
					address,
					dbService->getDeviceMetadata(address)
				);
				for (const auto &[_, sensor] : sensors) {
					auto type = sensor.getType();
					if (quantityIds.count(type) > 0) {
						continue;
					}
					auto quantity = dbService->getQuantityByType(type);
					if (!quantity) {
						continue;
					}
					quantityIds.insert_or_assign(type, quantity->m_id);
				}
			}
		}

		/**
		 * Populates response document with sensors response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Value array(kArrayType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			for (auto &[address, sensorVector] : devSenMap) {
				Value deviceObject;
				Pointer("/nAdr").Set(deviceObject, address, allocator);

				Value sensorArray(kArrayType);

				for (auto &[deviceSensor, sensor] : sensorVector) {
					Value sensorObject;
					Pointer("/idx").Set(sensorObject, deviceSensor.getGlobalIndex(), allocator);
					auto type = sensor.getType();
					if (quantityIds.count(type) > 0) {
						Pointer("/id").Set(sensorObject, quantityIds[type], allocator);
					} else {
						Pointer("/id").Set(sensorObject, "", allocator);
					}
					Pointer("/type").Set(sensorObject, sensor.getType(), allocator);
					Pointer("/name").Set(sensorObject, sensor.getName(), allocator);
					Pointer("/shortName").Set(sensorObject, sensor.getShortname(), allocator);
					Pointer("/unit").Set(sensorObject, sensor.getUnit(), allocator);
					Pointer("/decimalPlaces").Set(sensorObject, sensor.getDecimals(), allocator);
					Value frcs(kArrayType);
					for (const auto &cmd : sensor.getImplementedFrcs()) {
						frcs.PushBack(Value(cmd).Move(), allocator);
					}
					Pointer("/frcs").Set(sensorObject, frcs, allocator);
					sensorArray.PushBack(sensorObject, allocator);
				}

				Pointer("/sensors").Set(deviceObject, sensorArray, allocator);

				if (includeMetadata) {
					if (metadata.count(address) == 0 || metadata[address] == nullptr) {
						deviceObject.AddMember("metaData", rapidjson::Value(kNullType), allocator);
					} else {
						Document metadataDoc(kObjectType);
						metadataDoc.Parse(metadata[address]->c_str());
						deviceObject.AddMember("metaData", rapidjson::Value(metadataDoc, allocator).Move(), allocator);
					}
				}

				array.PushBack(deviceObject, allocator);
			}

			Pointer("/data/rsp/sensorDevices").Set(doc, array, allocator);
			BaseMsg::createResponsePayload(doc);
		}
	private:
		/// Include metadata in response
		bool includeMetadata = false;
		/// Map of sensor device tuples
		std::map<uint8_t, std::vector<std::pair<DeviceSensor, Sensor>>> devSenMap;
		/// Map of device addresses and metadata
		std::map<uint8_t, std::shared_ptr<std::string>> metadata;
		/// Map of sensor types and string identifiers
		std::map<uint8_t, std::string> quantityIds;
	};
}
