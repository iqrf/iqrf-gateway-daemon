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


#include "ActionRecord.h"
#include "ActionRecordParser.h"
#include "Calibration.h"
#include "CalibrationParser.h"
#include "HwpidVersionsParser.h"
#include "Light.h"
#include "LightParser.h"
#include "Metadata.h"
#include "MetadataParser.h"
#include "Mfgc.h"
#include "MfgcParser.h"
#include "MinMaxDiag.h"
#include "MinMaxDiagParser.h"
#include "Nfcc.h"
#include "NfccParser.h"
#include "PersistentQuantity.h"
#include "PersistentQuantityParser.h"
#include "PowerSupplyParser.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace iqrf::metadata {

  Metadata MetadataParser::parseV0(const nlohmann::json& doc) {
    checkRequired(doc);

    return Metadata(
      HwpidVersionsParser::parse(doc["hwpidVersions"]),
      doc["routing"].get<bool>(),
      doc["beaming"].get<bool>(),
      doc["repeater"].get<bool>(),
      doc["frcAggregation"].get<bool>(),
      doc["iqarosCompatible"].get<bool>(),
      doc["iqrfSensor"].get<std::vector<uint8_t>>(),
      doc["iqrfBinaryOutput"].get<uint8_t>(),
      std::nullopt,
      std::nullopt,
      PowerSupplyParser::parseV0(doc["powerSupply"]),
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      std::nullopt
    );
  }

  Metadata MetadataParser::parseV1(const nlohmann::json& doc) {
    checkRequired(doc);

    std::optional<Light> light = std::nullopt;
    if (doc.contains("iqrfLight")) {
      light = LightParser::parse(doc["iqrfLight"]);
    }
    std::optional<MinMaxDiag> minMaxDiag = std::nullopt;
    if (doc.contains("minMaxDiag")) {
      minMaxDiag = MinMaxDiagParser::parse(doc["minMaxDiag"]);
    }
    std::optional<ActionRecord> actionRecord = std::nullopt;
    if (doc.contains("actionRecord")) {
      actionRecord = ActionRecordParser::parse(doc["actionRecord"]);
    }
    std::vector<PersistentQuantity> persistentQuantities = {};
    if (doc.contains("persistentQuantities")) {
      for (const auto& item : doc["persistentQuantities"]) {
        persistentQuantities.push_back(
          PersistentQuantityParser::parse(item)
        );
      }
    }
    std::optional<Nfcc> nfcc = std::nullopt;
    if (doc.contains("nfcc")) {
      nfcc = NfccParser::parse(doc["nfcc"]);
    }
    std::optional<Mfgc> mfgc = std::nullopt;
    if (doc.contains("mfgc")) {
      mfgc = MfgcParser::parse(doc["mfgc"]);
    }
    std::optional<Calibration> calibration = std::nullopt;
    if (doc.contains("calibration")) {
      calibration = CalibrationParser::parse(doc["calibration"]);
    }

    return Metadata(
      HwpidVersionsParser::parse(doc["hwpidVersions"]),
      doc["routing"].get<bool>(),
      doc["beaming"].get<bool>(),
      doc["repeater"].get<bool>(),
      doc["frcAggregation"].get<bool>(),
      doc["iqarosCompatible"].get<bool>(),
      doc["iqrfSensor"].get<std::vector<uint8_t>>(),
      doc["iqrfBinaryOutput"].get<uint8_t>(),
      std::move(light),
      std::move(minMaxDiag),
      PowerSupplyParser::parseV1(doc["powerSupply"]),
      std::move(actionRecord),
      std::move(persistentQuantities),
      std::move(nfcc),
      std::move(mfgc),
      std::move(calibration)
    );
  }

  void MetadataParser::checkRequired(const nlohmann::json& doc) {
    if (!doc.contains("hwpidVersions")) {
      throw std::invalid_argument("hwpidVersion definition missing in metadata definition.");
    }
    if (!doc.contains("routing")) {
      throw std::invalid_argument("routing field missing in metadata definition.");
    }
    if (!doc.contains("beaming")) {
      throw std::invalid_argument("beaming field missing in metadata definition.");
    }
    if (!doc.contains("repeater")) {
      throw std::invalid_argument("repeater field missing in metadata definition.");
    }
    if (!doc.contains("frcAggregation")) {
      throw std::invalid_argument("frcAggregation field missing in metadata definition.");
    }
    if (!doc.contains("iqarosCompatible")) {
      throw std::invalid_argument("iqarosCompatible field missing in metadata definition.");
    }
    if (!doc.contains("iqrfSensor")) {
      throw std::invalid_argument("iqrfSensor field missing in metadata definition.");
    }
    if (!doc.contains("iqrfBinaryOutput")) {
      throw std::invalid_argument("iqrfBinaryOutput field missing in metadata definition.");
    }
    if (!doc.contains("powerSupply")) {
      throw std::invalid_argument("powerSupply field missing in metadata definition.");
    }
  }

}
