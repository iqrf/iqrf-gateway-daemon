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

#include "GetSensorsMsg.h"

namespace iqrf {

	void GetSensorsMsg::handleMsg(IIqrfDb *dbService) {
		sensors = dbService->getSensors();
	}

	void GetSensorsMsg::createResponsePayload(Document &doc) {
		Value array(kArrayType);
		Document::AllocatorType &allocator = doc.GetAllocator();

		for (auto &item : sensors) {
			Value deviceObject;
			Pointer("/address").Set(deviceObject, item.first, allocator);

			std::vector<std::tuple<DeviceSensor, Sensor>> sensorVector = item.second;
			Value sensorArray(kArrayType);

			for (auto &s : sensorVector) {
				DeviceSensor deviceSensor = std::get<0>(s);
				Sensor sensor = std::get<1>(s);
				Value sensorObject;
				Pointer("/index").Set(sensorObject, deviceSensor.getIndex(), allocator);
				Pointer("/type").Set(sensorObject, sensor.getType(), allocator);
				Pointer("/name").Set(sensorObject, sensor.getName(), allocator);
				Pointer("/shortname").Set(sensorObject, sensor.getShortname(), allocator);
				Pointer("/unit").Set(sensorObject, sensor.getUnit(), allocator);
				Pointer("/decimalPlaces").Set(sensorObject, sensor.getDecimals(), allocator);
				Pointer("/frc2Bit").Set(sensorObject, sensor.hasFrc2Bit(), allocator);
				Pointer("/frc1Byte").Set(sensorObject, sensor.hasFrc1Byte(), allocator);
				Pointer("/frc2Byte").Set(sensorObject, sensor.hasFrc2Byte(), allocator);
				Pointer("/frc4Byte").Set(sensorObject, sensor.hasFrc4Byte(), allocator);
				std::shared_ptr<double> val = deviceSensor.getValue();
				if (val) {
					Pointer("/value").Set(sensorObject, *val.get(), allocator);
				} else {
					Pointer("/value").Create(sensorObject, allocator);
				}
				std::shared_ptr<std::string> updated = deviceSensor.getUpdated();
				if (updated) {
					Pointer("/updated").Set(sensorObject, *updated.get(), allocator);
				} else {
					Pointer("/updated").Create(sensorObject, allocator);
				}
				std::shared_ptr<std::string> metadata = deviceSensor.getMetadata();
				if (metadata) {
					Document metadataDoc;
					metadataDoc.Parse(metadata->c_str());
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
		BaseMsg::createResponsePayload(doc);
	}
}
