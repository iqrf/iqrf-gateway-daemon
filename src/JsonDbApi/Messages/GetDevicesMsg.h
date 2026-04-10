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
#include "product.hpp"

#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

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
      v = Pointer("/data/req/light").Get(doc);
      if (v) {
        includeLights = v->GetBool();
      }
    }

    /**
     * Handles get devices request
     * @param dbService IQRF DB service
     */
    void handleMsg(IIqrfDb *dbService) override {
      devices = dbService->getDevices(requestedDevices);
      std::set<uint32_t> productIds = {};
      for (const auto& device : devices) {
        productIds.insert(device.getProductId());
      }
      products = dbService->getProductsMap(productIds);
      if (includeSensors) {
        sensors = dbService->getDeviceAddressIndexSensorMap(requestedDevices);
      }
      if (includeBinouts || includeLights) {
        std::vector<uint32_t> deviceIds = {};
        for (const auto& device : devices) {
          deviceIds.push_back(device.getId());
        }
        if (includeBinouts) {
          binouts = dbService->getBinaryOutputCountMap(deviceIds);
        }
        if (includeLights) {
          lights = dbService->getLightAddressesByDeviceIds(deviceIds);
        }
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

        for (const auto &device : devices) {
          if (!products.count(device.getProductId())) {
            throw std::runtime_error("Product record for device missing.");
          }
          const auto& product = products.at(device.getProductId());
          Value object;
          Pointer("/address").Set(object, device.getAddress(), allocator);
          Pointer("/hwpid").Set(object, product.getHwpid(), allocator);
          if (!brief) {
            auto manufacturer = product.getManufacturer();
            if (manufacturer.has_value()) {
              Pointer("/manufacturer").Set(object, manufacturer.value(), allocator);
            } else {
              Pointer("/manufacturer").Create(object, allocator);
            }
            auto productName = product.getName();
            if (productName.has_value()) {
              Pointer("/product").Set(object, productName.value(), allocator);
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

          // lights
          if (includeLights) {
            Pointer("/light").Set(object, lights.find(device.getAddress()) != lights.end(), allocator);
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
    /// Include lights in response
    bool includeLights = false;
    /// Vector of requested device addresses
    std::vector<uint8_t> requestedDevices;
    /// Vector of device
    std::vector<Device> devices;
    /// Map of product IDs and products
    std::unordered_map<uint32_t, Product> products;
    /// Map of sensor device tuples
    std::map<uint8_t, std::vector<std::pair<uint8_t, Sensor>>> sensors;
    /// Map of binout devices
    std::map<uint8_t, uint8_t> binouts;
    /// Vector of light device addresses
    std::unordered_set<uint8_t> lights;
  };
}
