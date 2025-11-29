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
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::models {

/**
 * IQRF DB driver entity
 */
class Driver {
public:
  /**
   * Base constructor
   */
  Driver() = default;

  /**
   * Constructor without ID
   * @param name Driver name
   * @param peripheralNumber Driver peripheral number
   * @param version Driver version
   * @param versionFlags Driver version flags
   * @param driver Driver code
   * @param driverHash Driver hash
   */
  Driver(const std::string& name, const int16_t peripheralNumber, const double version, const uint8_t versionFlags,
    const std::string& driver, const std::string& driverHash)
    : name(name),
      peripheralNumber(peripheralNumber),
      version(version),
      versionFlags(versionFlags),
      driver(driver),
      driverHash(driverHash) {}

  /**
   * Full constructor
   * @param id Driver ID
   * @param name Driver name
   * @param peripheralNumber Driver peripheral number
   * @param version Driver version
   * @param versionFlags Driver version flags
   * @param driver Driver code
   * @param driverHash Driver hash
   */
  Driver(const uint32_t id, const std::string& name, const int16_t peripheralNumber, const double version, const uint8_t versionFlags,
    const std::string& driver, const std::string& driverHash)
    : id(id),
      name(name),
      peripheralNumber(peripheralNumber),
      version(version),
      versionFlags(versionFlags),
      driver(driver),
      driverHash(driverHash) {}

  /**
   * Returns driver ID
   * @return Driver ID
   */
  uint32_t getId() const {
    return id;
  }

  /**
   * Sets driver ID
   * @param id Driver ID
   */
  void setId(const uint32_t id) {
    this->id = id;
  };

  /**
   * Returns driver name
   * @return Driver name
   */
  const std::string& getName() const {
    return name;
  }

  /**
   * Sets driver name
   * @param name Driver name
   */
  void setName(const std::string& name) {
    this->name = name;
  }

  /**
   * Returns driver peripheral number
   * @return Driver peripheral number
   */
  int16_t getPeripheralNumber() const {
    return peripheralNumber;
  }

  /**
   * Sets driver peripheral number
   * @param peripheralNumber Driver peripheral number
   */
  void setPeripheralNumber(const int16_t peripheralNumber) {
    this->peripheralNumber = peripheralNumber;
  }

  /**
   * Returns driver version
   * @return Driver version
   */
  double getVersion() const {
    return version;
  }

  /**
   * Sets driver version
   * @param version Driver version
   */
  void setVersion(const double version) {
    this->version = version;
  }

  /**
   * Returns driver version flags
   * @return Driver version flags
   */
  uint8_t getVersionFlags() const {
    return versionFlags;
  }

  /**
   * Sets driver version flags
   * @param versionFlags Driver version flags
   */
  void setVersionFlags(const uint8_t versionFlags) {
    this->versionFlags = versionFlags;
  }

  /**
   * Returns driver code
   * @return Driver code
   */
  const std::string& getDriver() const {
    return driver;
  }

  /**
   * Sets driver code
   * @param driver Driver code
   */
  void setDriver(const std::string& driver) {
    this->driver = driver;
  }

  /**
   * Returns driver hash
   * @return Driver hash
   */
  const std::string& getDriverHash() const {
    return driverHash;
  }

  /**
   * Sets driver hash
   * @param driverHash Driver hash
   */
  void setDriverHash(const std::string& driverHash) {
    this->driverHash = driverHash;
  }

  /**
   * @brief Creates a Driver object from SQLite::Statement query result.
   *
   * @param stmt SQLiteCpp statement object
   * @return A new `Driver` constructed from query result.
   */
  static Driver fromResult(SQLite::Statement &stmt) {
    auto id = stmt.getColumn(0).getUInt();
    auto name = stmt.getColumn(1).getString();
    auto peripheralNumber = static_cast<int16_t>(stmt.getColumn(2).getInt());
    auto version = stmt.getColumn(3).getDouble();
    auto versionFlags = static_cast<uint8_t>(stmt.getColumn(4).getUInt());
    auto driver = stmt.getColumn(5).getString();
    auto driverHash = stmt.getColumn(6).getString();
    return Driver(id, name, peripheralNumber, version, versionFlags, driver, driverHash);
  }
private:
  /// Driver ID
  uint32_t id;
  /// Driver name
  std::string name;
  /// Driver peripheral number
  int16_t peripheralNumber;
  /// Driver version
  double version;
  /// Driver version flags
  uint8_t versionFlags;
  /// Driver content
  std::string driver;
  /// Driver hash
  std::string driverHash;
};

}

