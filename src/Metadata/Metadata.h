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

#include "ActionRecord.h"
#include "Calibration.h"
#include "HwpidVersions.h"
#include "Light.h"
#include "Mfgc.h"
#include "MinMaxDiag.h"
#include "Nfcc.h"
#include "PersistentQuantity.h"
#include "PowerSupply.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace iqrf::metadata {

  /**
   * Singular metadata object
   *
   * Objects of this class is returned regardless of which
   * version of metadata is being parsed.
   *
   * If some property is not present in a particular version,
   * the property getters are still present in this object,
   * but return `std::nullopt`.
   *
   * As of version 1, getters for new properties return `std::nullopt`
   * if a product does not have a specific property or implement a feature.
   */
  class Metadata {
   public:
    /**
     * Constructs metadata object
     *
     * @param versions HWPID versions
     * @param routing Device routes packets
     * @param beaming Device operates in beaming mode
     * @param repeater Device is a repeater
     * @param frcAggregation Device aggregates FRC data from beaming devices
     * @param iqarosCompatible Device is compatible with IQAROS system
     * @param sensors Sensors implemented by device
     * @param binaryOutputs Number of implemented binary outputs
     * @param light Light standard implementation
     * @param minMaxDiag Minimum and maximum sensor diagnostics data
     * @param powerSupply Power supply information
     * @param actionRecord User actions implemented by device
     * @param persistentQuantities Persistent quantity values stored in memory
     * @param nfcc Implements NFC communication
     * @param mfgc Implements manufacturing test communication
     * @param calibration Calibration data
     */
    Metadata(
      HwpidVersions versions,
      bool routing,
      bool beaming,
      bool repeater,
      bool frcAggregation,
      bool iqarosCompatible,
      std::vector<uint8_t> sensors,
      uint8_t binaryOutputs,
      std::optional<Light> light,
      std::optional<MinMaxDiag> minMaxDiag,
      PowerSupply powerSupply,
      std::optional<ActionRecord> actionRecord,
      std::vector<PersistentQuantity> peristentQuantities,
      std::optional<Nfcc> nfcc,
      std::optional<Mfgc> mfgc,
      std::optional<Calibration> calibration
    ): versions_(std::move(versions)),
      routing_(routing),
      beaming_(beaming),
      repeater_(repeater),
      frcAggregation_(frcAggregation),
      iqarosCompatible_(iqarosCompatible),
      sensors_(std::move(sensors)),
      binaryOutput_(binaryOutputs),
      light_(std::move(light)),
      minMaxDiag_(std::move(minMaxDiag)),
      powerSupply_(std::move(powerSupply)),
      actionRecord_(std::move(actionRecord)),
      peristentQuantities_(std::move(peristentQuantities)),
      nfcc_(std::move(nfcc)),
      mfgc_(std::move(mfgc)),
      calibration_(std::move(calibration)) {}

    /**
     * Get HWPID versions
     *
     * Contains minimum and maximum versions of HWPID
     * for a given product.
     *
     * The range specifies revisions of product this
     * particular profile is applicable to.
     *
     * @return HWPID versions
     */
    const HwpidVersions& versions() const { return versions_; }

    /**
     * Indicates whether device routes packets in network
     *
     * This property will always be present while metadata version 0
     * is supported for backwards compatibility.
     *
     * @return `true` if device routes packets, `false` otherwise
     */
    bool routing() const { return routing_; }

    /**
     * Indicates whether device operates in beaming mode
     *
     * This property will always be present while metadata version 0
     * is supported for backwards compatibility.
     *
     * @return `true` if device operates in beaming mode, `false` otherwise
     */
    bool beaming() const { return beaming_; }

    /**
     * Indicates whether device is a repeater
     *
     * This property will always be present while metadata version 0
     * is supported for backwards compatibility.
     *
     * @return `true` if device is a repeater, `false` otherwise
     */
    bool repeater() const { return repeater_; }

    /**
     * Indicates whether device aggregates FRC data
     *
     * This property will always be present while metadata version 0
     * is supported for backwards compatibility.
     *
     * @return `true` if device aggregates FRC data, `false` otherwise
     */
    bool frcAggregation() const { return frcAggregation_; }

    /**
     * Indicates whether device is compatible with IQAROS system
     *
     * This property will always be present while metadata version 0
     * is supported for backwards compatibility.
     *
     * @return `true` if device is compatible with IQAROS system, `false` otherwise
     */
    bool iqarosCompatible() const { return iqarosCompatible_; }

    /**
     * Get sensors implemented by device
     *
     * This property will always be present while metadata version 0
     * is supported for backwards compatibility.
     *
     * Within metadata, sensors are represented by the sensor type
     * value corresponding with measured quantities in the IQRF Sensor standard.
     * Empty vector means that device does not implement the standard.
     *
     * @return Sensors implemented by device
     */
    const std::vector<uint8_t>& sensors() const { return sensors_; }

    /**
     * Get binary outputs implemented by device
     *
     * This property will always be present while metadata version 0
     * is supported for backwards compatibility.
     *
     * Within metadata, device implementing the IQRF BinaryOutput sensor
     * is represented by the number of binary outputs present.
     * Value 0 means that device does not implement the standard.
     *
     * @return Binary outputs implemented by device
     */
    uint8_t binaryOutputs() const { return binaryOutput_; }

    /**
     * Get light implementation of device
     *
     * This property was introduced in version 1.
     *
     * Within metadata, device implementing the IQRF Light standard
     * is represented by an object with properties.
     * `std::nullopt` means that device does not implement the standard.
     *
     * @return Light implementation of device
     */
    const std::optional<Light>& light() const { return light_; }

    /**
     * Get min-max diagonstics implementation
     *
     * This property was introduced in version 1.
     *
     * Min-max diagnostic feature allows the device
     * to record measured historical minimum and maximum
     * values of certain quantities according to the
     * IQRF Sensor standard.
     *
     * @return Min-max diagnostics implementation
     */
    const std::optional<MinMaxDiag>& minMaxDiag() const { return minMaxDiag_; }

    /**
     * Get device power supply information
     *
     * This property will always be present while metadata version 0
     * is supported for backwards compatibility.
     *
     * The power supply object contains information and statistics
     * for powering the device.
     *
     * @return Device power supply information
     */
    const PowerSupply& powerSupply() const { return powerSupply_; }

    /**
     * Get user actions implemented by device
     *
     * This property was introduced in version 1.
     *
     * @return User actions implemented by device
     */
    const std::optional<ActionRecord>& actionRecord() const { return actionRecord_; }

    /**
     * Get persistent quantity data
     *
     * This property was introduced in version 1.
     *
     * Devices may contain IQRF Sensor standard quantity
     * values stored persistently in a specific segment of memory.
     * This object provides information about which quantity values are stored,
     * the type of memory and address in memory of their location.
     *
     * @return Persistent quantity data
     */
    const std::vector<PersistentQuantity>& peristentQuantities() const { return peristentQuantities_; }

    /**
     * Get NFC communication specification
     *
     * This property was introduced in version 1.
     *
     * @return NFC communication specification
     */
    const std::optional<Nfcc>& nfcc() const { return nfcc_; }

    /**
     * Get manufacturing test communication specification
     *
     * This property was introduced in version 1.
     *
     * @return Manufacturing test communication specification
     */
    const std::optional<Mfgc>& mfgc() const { return mfgc_; }

    /**
     * Get calibration data specification
     *
     * This property was introduced in version 1.
     *
     * @return Calibration data specification
     */
    const std::optional<Calibration>& calibration() const { return calibration_; }

   private:
    /// HWPID versions
    HwpidVersions versions_;
    /// Device routes packets
    bool routing_;
    /// Device operates in beaming mode
    bool beaming_;
    /// Device is a repeater
    bool repeater_;
    /// Device aggregates FRC data from beaming devices
    bool frcAggregation_;
    /// Device is compatible with IQAROS system
    bool iqarosCompatible_;
    /// Sensors implemented by device
    std::vector<uint8_t> sensors_;
    /// Number of implemented binary outputs
    uint8_t binaryOutput_;
    /// Light standard implementation
    std::optional<Light> light_;
    /// Minimum and maximum sensor diagnostics data
    std::optional<MinMaxDiag> minMaxDiag_;
    /// Power supply information
    PowerSupply powerSupply_;
    /// User actions implemented by device
    std::optional<ActionRecord> actionRecord_;
    /// Persistent quantity values stored in memory
    std::vector<PersistentQuantity> peristentQuantities_;
    /// Implements NFC communication
    std::optional<Nfcc> nfcc_;
    /// Implements manufacturing test communication
    std::optional<Mfgc> mfgc_;
    /// Calibration data
    std::optional<Calibration> calibration_;
  };
}  // iqrf namespace
