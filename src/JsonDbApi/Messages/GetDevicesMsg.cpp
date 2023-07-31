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
			binouts = dbService->getBinaryOutputs();
		}
	}

	void GetDevicesMsg::createResponsePayload(Document &doc) {
		Value array(kArrayType);
		Document::AllocatorType &allocator = doc.GetAllocator();

		for (auto &item : devices) {
			Value object;
			Device device = std::get<0>(item);
			Pointer("/address").Set(object, device.getAddress(), allocator);
			Pointer("/hwpid").Set(object, std::get<1>(item), allocator);
			if (!brief) {
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
				Pointer("/hwpidVersion").Set(object, std::get<2>(item), allocator);
				Pointer("/osBuild").Set(object, std::get<3>(item), allocator);
				Pointer("/osVersion").Set(object, std::get<4>(item), allocator);
				Pointer("/dpa").Set(object, std::get<5>(item), allocator);
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

			// sensor
			if (includeSensors) {
				if (sensors.find(device.getAddress()) != sensors.end()) {
					Value sensorArray(kArrayType);
					auto sensorVector = sensors[device.getAddress()];
					for (auto &[ds, s] : sensorVector) {
						Value sensorObject;
						Pointer("/index").Set(sensorObject, ds.getGlobalIndex(), allocator);
						Pointer("/type").Set(sensorObject, s.getType(), allocator);
						Pointer("/name").Set(sensorObject, s.getName(), allocator);
						sensorArray.PushBack(sensorObject, allocator);
					}
					Pointer("/sensors").Set(object, sensorArray, allocator);
				} else {
					Pointer("/sensors").Create(object, allocator);
				}

			}

			// binout
			if (includeBinouts) {
				if (binouts.find(device.getAddress()) == binouts.end()) {
					Pointer("/binouts").Create(object, allocator);
				} else {
					Value boObject;
					auto bo = binouts[device.getAddress()];
					Pointer("/count").Set(boObject, bo, allocator);
					Pointer("/binouts").Set(object, boObject, allocator);
				}
			}

			array.PushBack(object, allocator);
		}

		Pointer("/data/rsp/devices").Set(doc, array, allocator);
		BaseMsg::createResponsePayload(doc);
	}
}
