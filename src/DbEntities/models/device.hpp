/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::models {

/**
 * IQRF DB device entity
 */
class Device {
public:
  /**
   * Base constructor
   */
  Device() = default;

  /**
   * Constructor without ID
   * @param address Network address
   * @param discovered Device is discovered
   * @param mid Module ID
   * @param productId Product ID
   * @param vrn Virtual routing number
   * @param zone Zone
   * @param parent Parent
   * @param enumerated Device is enumerated
   * @param metadata User metadata
   */
  Device(const uint8_t address, bool discovered, const uint32_t mid, const uint32_t productId, const uint8_t vrn, const uint8_t zone,
    std::optional<uint8_t> parent = std::nullopt, bool enumerated = false, std::shared_ptr<std::string> metadata = nullptr)
    : address(address),
      discovered(discovered),
      mid(mid),
      vrn(vrn),
      zone(zone),
      parent(parent),
      enumerated(enumerated),
      productId(productId),
      metadata(metadata) {}

  /**
   * Constructor
   * @param id ID
   * @param address Network address
   * @param discovered Device is discovered
   * @param mid Module ID
   * @param productId Product ID
   * @param vrn Virtual routing number
   * @param zone Zone
   * @param parent Parent
   * @param enumerated Device is enumerated
   * @param metadata User metadata
   */
  Device(const uint32_t id, const uint8_t address, bool discovered, const uint32_t mid, const uint32_t productId, const uint8_t vrn, const uint8_t zone,
    std::optional<uint8_t> parent = std::nullopt, bool enumerated = false, std::shared_ptr<std::string> metadata = nullptr)
    : id(id),
      address(address),
      discovered(discovered),
      mid(mid),
      vrn(vrn),
      zone(zone),
      parent(parent),
      enumerated(enumerated),
      productId(productId),
      metadata(metadata) {}

  /**
   * Destructor
   */
  ~Device() = default;

  /**
   * Returns device ID
   * @return Device ID
   */
  uint32_t getId() const {
    return id;
  }

  /**
   * Sets device ID
   * @param id Device ID
   */
  void setId(const uint32_t id) {
    this->id = id;
  };

  /**
   * Returns device network address
   * @return Device network address
   */
  uint8_t getAddress() const {
    return address;
  }

  /**
   * Sets device network address
   * @param address Device network address
   */
  void setAddress(const uint8_t address) {
    this->address = address;
  }

  /**
   * Returns device discovered status
   * @returns true if device is discovered, false otherwise
   */
  bool isDiscovered() const {
    return discovered;
  }

  /**
   * Sets device discovered status
   * @param discovered Device discovered status
   */
  void setDiscovered(bool discovered) {
    this->discovered = discovered;
  }

  /**
   * Returns device module ID
   * @return Device module ID
   */
  uint32_t getMid() const {
    return mid;
  }

  /**
   * Sets device module ID
   * @param mid Device module ID
   */
  void setMid(const uint32_t mid) {
    this->mid = mid;
  }

  /**
   * Returns device virtual routing number
   * @return Device virtual routing number
   */
  uint8_t getVrn() const {
    return vrn;
  }

  /**
   * Sets device virtual routing number
   * @param vrn Device virtual routing number
   */
  void setVrn(const uint8_t vrn) {
    this->vrn = vrn;
  }

  /**
   * Returns device zone
   * @return Device zone
   */
  uint8_t getZone() const {
    return zone;
  }

  /**
   * Sets device zone
   * @param zone Device zone
   */
  void setZone(const uint8_t zone) {
    this->zone = zone;
  }

  /**
   * Returns device parent address
   * @return Device parent address if discovered, otherwise null
   */
  std::optional<uint8_t> getParent() const {
    return parent;
  }

  /**
   * Sets device parent address
   * @param Device parent address
   */
  void setParent(std::optional<uint8_t> parent = std::nullopt) {
    this->parent = parent;
  }

  /**
   * Returns device enumerated status
   * @return true if device is enumerated, false otherwise
   */
  bool isEnumerated() const {
    return enumerated;
  }

  /**
   * Sets device enumerated status
   * @param enumerated Device enumerated status
   */
  void setEnumerated(bool enumerated) {
    this->enumerated = enumerated;
  }

  /**
   * Returns device product ID
   * @return Device product ID
   */
  uint32_t getProductId() const {
    return productId;
  }

  /**
   * Sets device product ID
   * @param productId Device product ID
   */
  void setProductId(const uint32_t productId) {
    this->productId = productId;
  }

  /**
   * Returns device metadata
   * @return Device metadata pointer
   */
  std::shared_ptr<std::string> getMetadata() const {
    return metadata;
  }

  /**
   * Sets device metadata
   * @param metadata Device metadata pointer
   */
  void setMetadata(std::shared_ptr<std::string> metadata) {
    this->metadata = std::move(metadata);
  }

  /**
   * Checks whether enumerated device is valid
   */
  bool isValid() {
    return mid > 0;
  }

  /**
   * @brief Creates a Device object from SQLite::Statement query result.
   *
   * @param stmt SQLiteCpp statement object
   * @return A new `Device` constructed from query result.
   */
  static Device fromResult(SQLite::Statement &stmt) {
    auto id = stmt.getColumn(0).getUInt();
    auto address = static_cast<uint8_t>(stmt.getColumn(1).getUInt());
    bool discovered = stmt.getColumn(2).getInt() != 0;
    auto mid = stmt.getColumn(3).getUInt();
    auto vrn = static_cast<uint8_t>(stmt.getColumn(4).getUInt());
    auto zone = static_cast<uint8_t>(stmt.getColumn(5).getUInt());
    std::optional<uint8_t> parent = std::nullopt;
    if (!stmt.getColumn(6).isNull()) {
      parent = static_cast<uint8_t>(stmt.getColumn(6).getUInt());
    }
    auto enumerated = stmt.getColumn(7).getUInt() != 0;
    auto productId = stmt.getColumn(8).getUInt();
    std::shared_ptr<std::string> metadata = nullptr;
    if (!stmt.getColumn(9).isNull()) {
      metadata = std::make_shared<std::string>(stmt.getColumn(9).getString());
    }
    return Device(id, address, discovered, mid, productId, vrn, zone, parent, enumerated, metadata);
  }
private:
  /// Device ID
  uint32_t id;
  /// Device address in network
  uint8_t address;
  /// Indicates whether device is discovered
  bool discovered;
  /// Module ID
  uint32_t mid;
  /// Device virtual routing number
  uint8_t vrn;
  /// Device zone
  uint8_t zone;
  /// Device parent
  std::optional<uint8_t> parent;
  /// Indicates whether device is enumerated
  bool enumerated;
  /// Product ID
  uint32_t productId;
  /// Device metadata
  std::shared_ptr<std::string> metadata;
};

}

