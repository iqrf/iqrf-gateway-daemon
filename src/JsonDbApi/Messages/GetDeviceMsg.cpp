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

#include "GetDeviceMsg.h"

namespace iqrf {

	GetDeviceMsg::GetDeviceMsg(const Document &doc) : BaseMsg(doc) {
		address = static_cast<uint8_t>(Pointer("/data/req/address").Get(doc)->GetUint());
		const Value *v = Pointer("/data/req/brief").Get(doc);
		if (v) {
			brief = v->GetBool();
		}
	}

	void GetDeviceMsg::handleMsg(IIqrfDb *dbService) {
		device = dbService->getDevice(address);
		product = dbService->getProductById(device.getProductId());
		if (dbService->hasSensors(device.getAddress())) {
			sensors = dbService->getDeviceSensorsByAddress(device.getAddress());
		}
	}

	void GetDeviceMsg::createResponsePayload(Document &doc) {
		if (m_status == 0) {
			Document::AllocatorType &allocator = doc.GetAllocator();

			Value object;
			Pointer("/address").Set(object, device.getAddress(), allocator);
			Pointer("/hwpid").Set(object, product.getHwpid(), allocator);
			if (!brief) {
				std::shared_ptr<std::string> productName = product.getName();
				if (productName) {
					Pointer("/product").Set(object, *productName.get(), allocator);
				} else {
					Pointer("/product").Create(object, allocator);
				}
				Pointer("/discovered").Set(object, device.isDiscovered(), allocator);
				Pointer("/vrn").Set(object, device.getVrn(), allocator);
				Pointer("/zone").Set(object, device.getZone(), allocator);
				std::shared_ptr<uint8_t> val = device.getParent();
				if (val) {
					Pointer("/parent").Set(object, *val.get(), allocator);
				} else {
					Pointer("/parent").Create(object, allocator);
				}
				Pointer("/mid").Set(object, device.getMid(), allocator);
				Pointer("/hwpidVersion").Set(object, product.getHwpidVersion(), allocator);
				Pointer("/osBuild").Set(object, product.getOsBuild(), allocator);
				Pointer("/osVersion").Set(object, product.getOsVersion(), allocator);
				Pointer("/dpa").Set(object, product.getDpaVersion(), allocator);

				/// sensors
				Value sensorArray(kArrayType);
				for (auto &[index, sensor] : sensors) {
					Value sensorObject;
					Pointer("/index").Set(sensorObject, index, allocator);
					Pointer("/type").Set(sensorObject, sensor.getType(), allocator);
					Pointer("/name").Set(sensorObject, sensor.getName(), allocator);
					Pointer("/shortname").Set(sensorObject, sensor.getShortname(), allocator);
					Pointer("/unit").Set(sensorObject, sensor.getUnit(), allocator);
					Pointer("/decimalPlaces").Set(sensorObject, sensor.getDecimals(), allocator);
					Value frcArray(kArrayType);
					if (sensor.hasFrc2Bit()) {
						frcArray.PushBack(iqrf::sensor::STD_SENSOR_FRC_2BITS, allocator);
					}
					if (sensor.hasFrc1Byte()) {
						frcArray.PushBack(iqrf::sensor::STD_SENSOR_FRC_1BYTE, allocator);
					}
					if (sensor.hasFrc2Byte()) {
						frcArray.PushBack(iqrf::sensor::STD_SENSOR_FRC_2BYTES, allocator);
					}
					if (sensor.hasFrc4Byte()) {
						frcArray.PushBack(iqrf::sensor::STD_SENSOR_FRC_4BYTES, allocator);
					}
					Pointer("/frcs").Set(sensorObject, frcArray, allocator);
					sensorArray.PushBack(sensorObject, allocator);
				}
				Pointer("/sensors").Set(object, sensorArray, allocator);
			}
			std::shared_ptr<std::string> val = device.getName();
			if (val) {
				Pointer("/name").Set(object, *val.get(), allocator);
			} else {
				Pointer("/name").Create(object, allocator);
			}
			val = device.getLocation();
			if (val) {
				Pointer("/location").Set(object, *val.get(), allocator);
			} else {
				Pointer("/location").Create(object, allocator);
			}

			Pointer("/data/rsp/device").Set(doc, object, allocator);
		} else {
			Pointer("/data/rsp/device").Set(doc, rapidjson::Value(kNullType));
		}
		BaseMsg::createResponsePayload(doc);
	}
}
