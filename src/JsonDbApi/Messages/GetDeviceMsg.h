/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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

#include "BaseMsg.h"

using namespace rapidjson;

namespace iqrf {

	/**
	 * Get device request message
	 */
	class GetDeviceMsg : public BaseMsg {
	public:
		GetDeviceMsg() = default;  // Default constructor

		GetDeviceMsg(const GetDeviceMsg&) = delete;  // Delete copy constructor

		GetDeviceMsg& operator=(const GetDeviceMsg&) = delete;  // Delete copy assignment operator

		GetDeviceMsg(GetDeviceMsg&&) = default;  // Default move constructor

		GetDeviceMsg& operator=(GetDeviceMsg&&) = default; // Default move assignment

		/**
		 * Constructor
		 * @param doc Request document
		 */
		GetDeviceMsg(const Document &doc) : BaseMsg(doc) {
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
      v = Pointer("/data/req/light").Get(doc);
      if (v) {
        includeLight = v->GetBool();
      }
		}

		/**
		 * Handles get devices request
		 * @param dbService IQRF DB service
		 */
		void handleMsg(IIqrfDb *dbService) override {
			device = dbService->getDeviceByAddress(address);
			if (device == nullptr) {
				throw std::logic_error("Device at address " + std::to_string(address) + " does not exist.");
			}
			product = dbService->getProduct(device->getProductId());
			if (product == nullptr) {
				throw std::logic_error("Product ID " + std::to_string(device->getProductId()) + " does not exist.");
			}
      if (includeSensors) {
        if (dbService->deviceImplementsPeripheral(device->getId(), PERIPHERAL_SENSOR)) {
          sensors = dbService->getDeviceSensorsMapByAddress(device->getAddress());
        }
      }
      if (includeBinouts) {
        auto dbBinout = dbService->getBinaryOutputByDeviceId(device->getId());
        if (dbBinout) {
          binouts = dbBinout->getCount();
        }
      }
      if (includeLight) {
        light = dbService->deviceImplementsPeripheral(device->getId(), PERIPHERAL_LIGHT);
      }
		}

		/**
		 * Populates response document with devices response
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			if (m_status == 0) {
				Document::AllocatorType &allocator = doc.GetAllocator();

				Value object;
				Pointer("/address").Set(object, device->getAddress(), allocator);
				Pointer("/hwpid").Set(object, product->getHwpid(), allocator);
				if (!brief) {
					std::shared_ptr<std::string> productName = product->getName();
					if (productName) {
						Pointer("/product").Set(object, *productName.get(), allocator);
					} else {
						Pointer("/product").Create(object, allocator);
					}
					Pointer("/discovered").Set(object, device->isDiscovered(), allocator);
					Pointer("/vrn").Set(object, device->getVrn(), allocator);
					Pointer("/zone").Set(object, device->getZone(), allocator);
					if (device->getParent().has_value()) {
						Pointer("/parent").Set(object, device->getParent().value(), allocator);
					} else {
						Pointer("/parent").Create(object, allocator);
					}
					Pointer("/mid").Set(object, device->getMid(), allocator);
					Pointer("/hwpidVersion").Set(object, product->getHwpidVersion(), allocator);
					Pointer("/osBuild").Set(object, product->getOsBuild(), allocator);
					Pointer("/osVersion").Set(object, product->getOsVersion(), allocator);
					Pointer("/dpa").Set(object, product->getDpaVersion(), allocator);
					/// metadata
					std::shared_ptr<std::string> metadata = device->getMetadata();
					if (metadata) {
						Document metadataDoc(kObjectType);
						metadataDoc.Parse(metadata.get()->c_str());
						object.AddMember("metadata", rapidjson::Value(metadataDoc, allocator).Move(), allocator);
					} else {
						object.AddMember("metadata", rapidjson::Value(kNullType), allocator);
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
						if (sensor.hasFrcByte()) {
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

        // light
        if (includeLight) {
          Pointer("/light").Set(object, light, allocator);
        }

				Pointer("/data/rsp/device").Set(doc, object, allocator);
			} else {
				Pointer("/data/rsp/device").Set(doc, rapidjson::Value(kNullType));
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
    /// Include light in response
    bool includeLight = false;
		// Device address
		uint8_t address;
		/// Vector of devices with product information
		std::unique_ptr<Device> device = nullptr;
		/// Product info
		std::unique_ptr<Product> product = nullptr;
		/// Sensors
		std::map<uint8_t, Sensor> sensors;
		/// Binout count
		uint8_t binouts = 0;
    /// Light implemented
    bool light = false;
	};
}
