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
	 * Get devices request message
	 */
	class GetDevicesMsg : public BaseMsg {
	public:
		/// Delete base constructor
		GetDevicesMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetDevicesMsg(const Document &doc) : BaseMsg(doc) {
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

		/**
		 * Handles get devices request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			devices = dbService->getDevices(requestedDevices);
			if (includeSensors) {
				sensors = dbService->getDeviceAddressIndexSensorMap();
			}
			if (includeBinouts) {
				binouts = dbService->getBinaryOutputCountMap();
			}
		}

		/**
		 * Populates response document with devices response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			if (m_status == 0) {
				Value array(kArrayType);
				Document::AllocatorType &allocator = doc.GetAllocator();

				for (auto &[device, product] : devices) {
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
						if (device.getParent().has_value()) {
							Pointer("/parent").Set(object, device.getParent().value(), allocator);
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
							for (auto &[idx, s] : sensorVector) {
								Value sensorObject;
								Pointer("/index").Set(sensorObject, idx, allocator);
								Pointer("/type").Set(sensorObject, s.getType(), allocator);
								Pointer("/name").Set(sensorObject, s.getName(), allocator);
								Pointer("/shortname").Set(sensorObject, s.getShortname(), allocator);
								Pointer("/unit").Set(sensorObject, s.getUnit(), allocator);
								Pointer("/decimalPlaces").Set(sensorObject, s.getDecimals(), allocator);
								Value frcArray(kArrayType);
								if (s.hasFrc2Bit()) {
									frcArray.PushBack(iqrf::sensor::STD_SENSOR_FRC_2BITS, allocator);
								}
								if (s.hasFrcByte()) {
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
			BaseMsg::createResponsePayload(doc);
		}
	private:
		/// Indicates whether response should contain brief information
		bool brief = false;
		/// Include binary outputs in response
		bool includeBinouts = false;
		/// Include sensors in response
		bool includeSensors = false;
		/// Vector of requested device addresses
		std::vector<uint8_t> requestedDevices;
		/// Vector of devices with product information
		std::vector<std::pair<Device, Product>> devices;
		/// Map of sensor device tuples
		std::map<uint8_t, std::vector<std::pair<uint8_t, Sensor>>> sensors;
		/// Map of binout devices
		std::map<uint8_t, uint8_t> binouts;
	};
}
