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

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::models {

/**
 * IQRF DB product entity
 */
class Product {
public:

  /**
   * Base constructor
   */
  Product() = default;

  /**
   * Constructor without ID
   * @param hwpid HWPID
   * @param hwpidVersion HWPID version
   * @param osBuild OS build
   * @param osVersion OS version
   * @param dpaVersion DPA version
   * @param handlerUrl Handler URL
   * @param handlerHash Handler hash
   * @param customDriver Custom product driver
   * @param packageId Package ID
   * @param manufacturer Product manufacturer
   * @param name Product name
   */
  Product(
    uint16_t hwpid,
    uint16_t hwpidVersion,
    uint16_t osBuild,
    const std::string& osVersion,
    uint16_t dpaVersion,
    std::optional<std::string> handlerUrl = std::nullopt,
    std::optional<std::string> handlerHash = std::nullopt,
    std::optional<std::string> customDriver = std::nullopt,
    std::optional<uint32_t> packageId = std::nullopt,
    std::optional<std::string> manufacturer = std::nullopt,
    std::optional<std::string> name = std::nullopt
  ): hwpid_(hwpid),
    hwpidVersion_(hwpidVersion),
    osBuild_(osBuild),
    osVersion_(osVersion),
    dpaVersion_(dpaVersion),
    handlerUrl_(std::move(handlerUrl)),
    handlerHash_(std::move(handlerHash)),
    customDriver_(std::move(customDriver)),
    packageId_(std::move(packageId)),
    manufacturer_(std::move(manufacturer)),
    name_(std::move(name)) {};

  /**
   * Full constructor
   * @param id ID
   * @param hwpid HWPID
   * @param hwpidVersion HWPID version
   * @param osBuild OS build
   * @param osVersion OS version
   * @param dpaVersion DPA version
   * @param handlerUrl Handler URL
   * @param handlerHash Handler hash
   * @param customDriver Custom product driver
   * @param packageId Package ID
   * @param manufacturer Product manufacturer
   * @param name Product name
   */
  Product(
    uint32_t id,
    uint16_t hwpid,
    uint16_t hwpidVersion,
    uint16_t osBuild,
    const std::string& osVersion,
    uint16_t dpaVersion,
    std::optional<std::string> handlerUrl = std::nullopt,
    std::optional<std::string> handlerHash = std::nullopt,
    std::optional<std::string> customDriver = std::nullopt,
    std::optional<uint32_t> packageId = std::nullopt,
    std::optional<std::string> manufacturer = std::nullopt,
    std::optional<std::string> name = std::nullopt
  ): id_(id),
    hwpid_(hwpid),
    hwpidVersion_(hwpidVersion),
    osBuild_(osBuild),
    osVersion_(osVersion),
    dpaVersion_(dpaVersion),
    handlerUrl_(std::move(handlerUrl)),
    handlerHash_(std::move(handlerHash)),
    customDriver_(std::move(customDriver)),
    packageId_(std::move(packageId)),
    manufacturer_(std::move(manufacturer)),
    name_(std::move(name)) {};

  /**
   * Returns product ID
   * @return Product ID
   */
  uint32_t getId() const {
    return id_;
  }

  /**
   * Sets product ID
   * @param id Product ID
   */
  void setId(uint32_t id) {
    this->id_ = id;
  }

  /**
   * Returns product HWPID
   * @return Product HWPID
   */
  uint16_t getHwpid() const {
    return hwpid_;
  }

  /**
   * Sets product HWPID
   * @param hwpid Product HWPID
   */
  void setHwpid(uint16_t hwpid) {
    this->hwpid_ = hwpid;
  }

  /**
   * Returns product HWPID version
   * @return Product HWPID version
   */
  uint16_t getHwpidVersion() const {
    return hwpidVersion_;
  }

  /**
   * Sets product HWPID version
   * @param hwpidVersion Product HWPID version
   */
  void setHwpidVersion(uint16_t hwpidVersion) {
    this->hwpidVersion_ = hwpidVersion;
  }

  /**
   * Returns product OS build
   * @return Product OS build
   */
  uint16_t getOsBuild() const {
    return osBuild_;
  }

  /**
   * Sets product OS build
   * @param osBuild Product OS build
   */
  void setOsBuild(uint16_t osBuild) {
    this->osBuild_ = osBuild;
  }

  /**
   * Returns product OS version
   * @return Product OS version
   */
  const std::string& getOsVersion() const {
    return osVersion_;
  }

  /**
   * Sets product OS version
   * @param osVersion Product OS version
   */
  void setOsVersion(const std::string& osVersion) {
    this->osVersion_ = osVersion;
  }

  /**
   * Returns product DPA version
   * @return Product DPA version
   */
  uint16_t getDpaVersion() const {
    return dpaVersion_;
  }

  /**
   * Sets product DPA version
   * @param dpaVersion Product DPA version
   */
  void setDpaVersion(uint16_t dpaVersion) {
    this->dpaVersion_ = dpaVersion;
  }

  /**
   * Returns product handler url
   * @return Product handler url
   */
  std::optional<std::string> getHandlerUrl() const {
    return handlerUrl_;
  }

  /**
   * Sets product hnadler url
   * @param handlerUrl Product handler url
   */
  void setHandlerUrl(std::optional<std::string> handlerUrl) {
    this->handlerUrl_ = std::move(handlerUrl);
  }

  /**
   * Returns product handler hash
   * @return Product handler hash
   */
  std::optional<std::string> getHandlerHash() const {
    return handlerHash_;
  }

  /**
   * Sets product handler hash
   * @param handlerHash Product handler hash
   */
  void setHandlerHash(std::optional<std::string> handlerHash) {
    this->handlerHash_ = std::move(handlerHash);
  }

  /**
   * Returns product custom driver
   * @return Product custom driver
   */
  std::optional<std::string> getCustomDriver() const {
    return customDriver_;
  }

  /**
   * Sets product custom driver
   * @param customDriver Product custom driver
   */
  void setCustomDriver(std::optional<std::string> customDriver) {
    this->customDriver_ = std::move(customDriver);
  }

  /**
   * Returns product package ID
   * @return Product package ID
   */
  std::optional<uint32_t> getPackageId() const {
    return packageId_;
  }

  /**
   * Sets product package ID
   * @param packageId Product package ID
   */
  void setPackageId(std::optional<uint32_t> packageId) {
    this->packageId_ = std::move(packageId);
  }

  /**
   * Returns product manufacturer
   * @return Product manufacturer
   */
  std::optional<std::string> getManufacturer() const {
    return manufacturer_;
  }

  /**
   * Sets product manufacturer
   * @param manufacturer Product manufacturer
   */
  void setManufacturer(std::optional<std::string> manufacturer) {
    this->manufacturer_ = std::move(manufacturer);
  }

  /**
   * Returns product name
   * @return Product name
   */
  std::optional<std::string> getName() const {
    return name_;
  }

  /**
   * Sets product name
   * @param name Product name
   */
  void setName(std::optional<std::string> name) {
    this->name_ = std::move(name);
  }

  /**
   * Checks whether enumerated product is valid
   */
  bool isValid() {
    return osBuild_ > 0 && dpaVersion_ > 0;
  }

  /**
   * @brief Creates a Product object from SQLite::Statement query result.
   *
   * @param stmt SQLiteCpp statement object
   * @return A new `Product` constructed from query result.
   */
  static Product fromResult(SQLite::Statement &stmt) {
    auto id = stmt.getColumn(0).getUInt();
    auto hwpid = static_cast<uint16_t>(stmt.getColumn(1).getUInt());
    auto hwpidVersion = static_cast<uint16_t>(stmt.getColumn(2).getUInt());
    auto osBuild = static_cast<uint16_t>(stmt.getColumn(3).getUInt());
    auto osVersion = stmt.getColumn(4).getString();
    auto dpaVersion = static_cast<uint16_t>(stmt.getColumn(5).getUInt());
    std::optional<std::string> handlerUrl = std::nullopt;
    if (!stmt.getColumn(6).isNull()) {
      handlerUrl = stmt.getColumn(6).getString();
    }
    std::optional<std::string> handlerHash = std::nullopt;
    if (!stmt.getColumn(7).isNull()) {
      handlerHash = stmt.getColumn(7).getString();
    }
    std::optional<std::string> customDriver = std::nullopt;
    if (!stmt.getColumn(8).isNull()) {
      customDriver = stmt.getColumn(8).getString();
    }
    std::optional<uint32_t> packageId = std::nullopt;
    if (!stmt.getColumn(9).isNull()) {
      packageId = stmt.getColumn(9).getUInt();
    }
    std::optional<std::string> manufacturer = std::nullopt;
    if (!stmt.getColumn(10).isNull()) {
      manufacturer = stmt.getColumn(10).getString();
    }
    std::optional<std::string> name = std::nullopt;
    if (!stmt.getColumn(11).isNull()) {
      name = stmt.getColumn(11).getString();
    }
    return Product(
      id,
      hwpid,
      hwpidVersion,
      osBuild,
      osVersion,
      dpaVersion,
      std::move(handlerUrl),
      std::move(handlerHash),
      std::move(customDriver),
      std::move(packageId),
      std::move(manufacturer),
      std::move(name)
    );
  }

  /// Set of driver IDs
  std::set<uint32_t> drivers;
private:
  /// Product ID
  uint32_t id_;
  /// Product HWPID
  uint16_t hwpid_;
  /// Product HWPID version
  uint16_t hwpidVersion_;
  /// Product OS build
  uint16_t osBuild_;
  /// Product OS version
  std::string osVersion_;
  /// Product DPA version
  uint16_t dpaVersion_;
  /// Product handler url
  std::optional<std::string> handlerUrl_;
  /// Product handler hash
  std::optional<std::string> handlerHash_;
  /// Product customDriver
  std::optional<std::string> customDriver_;
  /// Product package ID
  std::optional<uint32_t> packageId_;
  /// Product manufacturer
  std::optional<std::string> manufacturer_;
  /// Product name
  std::optional<std::string> name_;
};

}
