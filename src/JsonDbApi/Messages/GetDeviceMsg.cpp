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
		v = Pointer("/data/req/sensors").Get(doc);
		if (v) {
			includeSensors = v->GetBool();
		}
		v = Pointer("/data/req/binouts").Get(doc);
		if (v) {
			includeBinouts = v->GetBool();
		}
	}

	void GetDeviceMsg::handleMsg(IIqrfDb *dbService) {
		device = dbService->getDevice(address);
		product = dbService->getProductById(device.getProductId());
		if (dbService->hasSensors(device.getAddress())) {
			sensors = dbService->getDeviceSensorsByAddress(device.getAddress());
		}
		auto dbBinout = dbService->getBinaryOutputByDeviceId(device.getId());
		if (dbBinout) {
			binouts = dbBinout->getCount();
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
				std::shared_ptr<uint8_t> parent = device.getParent();
				if (parent) {
					Pointer("/parent").Set(object, *parent.get(), allocator);
				} else {
					Pointer("/parent").Create(object, allocator);
				}
				Pointer("/mid").Set(object, device.getMid(), allocator);
				Pointer("/hwpidVersion").Set(object, product.getHwpidVersion(), allocator);
				Pointer("/osBuild").Set(object, product.getOsBuild(), allocator);
				Pointer("/osVersion").Set(object, product.getOsVersion(), allocator);
				Pointer("/dpa").Set(object, product.getDpaVersion(), allocator);
				/// metadata
				std::shared_ptr<std::string> metadata = device.getMetadata();
				if (metadata) {
					Document metadataDoc;
					metadataDoc.Parse(metadata.get()->c_str());
					Pointer("/metadata").Set(object, metadataDoc, allocator);
				} else {
					Pointer("/metadata").Set(object, Value(kNullType), allocator);
				}
			}

			if (includeSensors) {
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

			// binouts
			if (includeBinouts) {
				Value boObject;
				Pointer("/count").Set(boObject, binouts, allocator);
				Pointer("/binouts").Set(object, boObject, allocator);
			}

			Pointer("/data/rsp/device").Set(doc, object, allocator);
		} else {
			Pointer("/data/rsp/device").Set(doc, rapidjson::Value(kNullType));
		}
		BaseMsg::createResponsePayload(doc);
	}
}
