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

  class Metadata {
   public:
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

    const HwpidVersions& versions() const { return versions_; }

    bool routing() const { return routing_; }

    bool beaming() const { return beaming_; }

    bool repeater() const { return repeater_; }

    bool frcAggregation() const { return frcAggregation_; }

    bool iqarosCompatible() const { return iqarosCompatible_; }

    const std::vector<uint8_t>& sensors() const { return sensors_; }

    uint8_t binaryOutputs() const { return binaryOutput_; }

    const std::optional<Light>& light() const { return light_; }

    const std::optional<MinMaxDiag>& minMaxDiag() const { return minMaxDiag_; }

    const PowerSupply& powerSupply() const { return powerSupply_; }

    const std::optional<ActionRecord>& actionRecord() const { return actionRecord_; }

    const std::vector<PersistentQuantity>& peristentQuantities() const { return peristentQuantities_; }

    const std::optional<Nfcc>& nfcc() const { return nfcc_; }

    const std::optional<Mfgc>& mfgc() const { return mfgc_; }

    const std::optional<Calibration>& calibration() const { return calibration_; }

   private:
    HwpidVersions versions_;
    bool routing_;
    bool beaming_;
    bool repeater_;
    bool frcAggregation_;
    bool iqarosCompatible_;
    std::vector<uint8_t> sensors_;
    uint8_t binaryOutput_;
    std::optional<Light> light_;
    std::optional<MinMaxDiag> minMaxDiag_;
    PowerSupply powerSupply_;
    std::optional<ActionRecord> actionRecord_;
    std::vector<PersistentQuantity> peristentQuantities_;
    std::optional<Nfcc> nfcc_;
    std::optional<Mfgc> mfgc_;
    std::optional<Calibration> calibration_;
  };
}  // iqrf namespace
