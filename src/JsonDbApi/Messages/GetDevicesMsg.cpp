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

#include "GetDevicesMsg.h"

namespace iqrf {

	GetDevicesMsg::GetDevicesMsg(const Document &doc) : BaseMsg(doc) {
		const Value *v = Pointer("/data/req/brief").Get(doc);
		if (v) {
			brief = v->GetBool();
		}
		v = Pointer("/data/req/addresses").Get(doc);
		if (v) {
			auto arr = v->GetArray();
			for (auto itr = arr.Begin(); itr != arr.End(); ++itr) {
				requestedDevices.push_back(static_cast<uint8_t>(itr->GetUint()));
			}
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

	void GetDevicesMsg::handleMsg(IIqrfDb *dbService) {
		devices = dbService->getDevices(requestedDevices);
		if (includeSensors) {
			sensors = dbService->getSensors();
		}
		if (includeBinouts) {
			binouts = dbService->getBinaryOutputCountMap();
		}
	}

	void GetDevicesMsg::createResponsePayload(Document &doc) {
		if (m_status == 0) {
			Value array(kArrayType);
			Document::AllocatorType &allocator = doc.GetAllocator();

			for (auto &item : devices) {
				Value object;
				Device device = std::get<0>(item);
				Product product = std::get<1>(item);
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
						object.AddMember("metadata", rapidjson::Value(metadataDoc, allocator).Move(), allocator);
					} else {
						object.AddMember("metadata", rapidjson::Value(kNullType), allocator);
					}
				}

				/// sensors
				if (includeSensors) {
					Value sensorArray(kArrayType);
					if (sensors.find(device.getAddress()) != sensors.end()) {
						auto sensorVector = sensors[device.getAddress()];
						for (auto &[ds, s] : sensorVector) {
							Value sensorObject;
							Pointer("/index").Set(sensorObject, ds.getGlobalIndex(), allocator);
							Pointer("/type").Set(sensorObject, s.getType(), allocator);
							Pointer("/name").Set(sensorObject, s.getName(), allocator);
							Pointer("/shortname").Set(sensorObject, s.getShortname(), allocator);
							Pointer("/unit").Set(sensorObject, s.getUnit(), allocator);
							Pointer("/decimalPlaces").Set(sensorObject, s.getDecimals(), allocator);
							Value frcArray(kArrayType);
							if (s.hasFrc2Bit()) {
								frcArray.PushBack(iqrf::sensor::STD_SENSOR_FRC_2BITS, allocator);
							}
							if (s.hasFrc1Byte()) {
								frcArray.PushBack(iqrf::sensor::STD_SENSOR_FRC_1BYTE, allocator);
							}
							if (s.hasFrc2Byte()) {
								frcArray.PushBack(iqrf::sensor::STD_SENSOR_FRC_2BYTES, allocator);
							}
							if (s.hasFrc4Byte()) {
								frcArray.PushBack(iqrf::sensor::STD_SENSOR_FRC_4BYTES, allocator);
							}
							Pointer("/frcs").Set(sensorObject, frcArray, allocator);
							sensorArray.PushBack(sensorObject, allocator);
						}
					}
					Pointer("/sensors").Set(object, sensorArray, allocator);
				}

				// binouts
				if (includeBinouts) {
					auto devBinout = binouts.find(device.getAddress());
					auto count = devBinout == binouts.end() ? 0 : devBinout->second;
					Value boObject;
					Pointer("/count").Set(boObject, count, allocator);
					Pointer("/binouts").Set(object, boObject, allocator);
				}

				array.PushBack(object, allocator);
			}

			Pointer("/data/rsp/devices").Set(doc, array, allocator);
		}
	}
}
