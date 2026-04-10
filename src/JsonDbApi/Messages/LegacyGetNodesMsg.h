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
   * Get nodes request message
   */
  class LegacyGetNodesMsg : public BaseMsg {
  public:
    /// Delete base constructor
    LegacyGetNodesMsg() = delete;

    /**
     * Constructor
     * @param doc Request document
     */
    LegacyGetNodesMsg(const Document &doc) : BaseMsg(doc) {}

    /**
     * Handles get devices request
     * @param dbService IQRF DB service
     */
    void handleMsg(IIqrfDb *dbService) override {
      includeMetadata = dbService->getMetadataToMessages();
      devices = dbService->getDevices();
      std::set<uint32_t> productIds = {};
      for (const auto& device : devices) {
        productIds.insert(device.getProductId());
      }
      products = dbService->getProductsMap(productIds);
    }

    /**
     * Populates response document with devices response
     * @param doc Response document
     */
    void createResponsePayload(Document &doc) override {
      Value array(kArrayType);
      Document::AllocatorType &allocator = doc.GetAllocator();

      for (auto &device : devices) {
        if (!products.count(device.getProductId())) {
          throw std::runtime_error("Product record for device missing.");
        }
        const auto& product = products.at(device.getProductId());
        Value object(kObjectType);

        Pointer("/nAdr").Set(object, device.getAddress(), allocator);
        Pointer("/mid").Set(object, device.getMid(), allocator);
        Pointer("/disc").Set(object, device.isDiscovered(), allocator);
        Pointer("/hwpid").Set(object, product.getHwpid(), allocator);
        Pointer("/hwpidVer").Set(object, product.getHwpidVersion(), allocator);
        Pointer("/osBuild").Set(object, product.getOsBuild(), allocator);
        Pointer("/dpaVer").Set(object, product.getDpaVersion(), allocator);
        if (includeMetadata) {
          std::shared_ptr<std::string> metadata = device.getMetadata();
          if (metadata) {
            Document metadataDoc;
            metadataDoc.Parse(metadata.get()->c_str());
            object.AddMember("metaData", rapidjson::Value(metadataDoc, allocator).Move(), allocator);
          } else {
            object.AddMember("metaData", rapidjson::Value(kNullType), allocator);
          }
        }

        array.PushBack(object, allocator);
      }

      Pointer("/data/rsp/nodes").Set(doc, array, allocator);
      BaseMsg::createResponsePayload(doc);
    }
  private:
    /// Include metadata in response
    bool includeMetadata = false;
    /// Vector of device
    std::vector<Device> devices;
    /// Map of product IDs and products
    std::unordered_map<uint32_t, Product> products;
  };
}
