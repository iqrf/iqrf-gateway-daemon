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

#include "Metadata.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace iqrf::metadata {

  class ProductMetadata {
   public:
    ProductMetadata(
      uint32_t version,
      std::vector<Metadata> profiles
    ): version_(version),
      profiles_(std::move(profiles)) {}

    uint32_t version() const { return version_; }

    const std::vector<Metadata>& profiles() const { return profiles_; }

    const Metadata* getProfile(uint32_t hwpidVersion) {
      for (const auto &profile : profiles_) {
        if (hwpidVersion >= profile.versions().min() &&
          (!profile.versions().max().has_value() || profile.versions().max().value() >= hwpidVersion)
        ) {
          return &profile;
        }
      }
      return nullptr;
    }

   private:
    uint32_t version_;
    std::vector<Metadata> profiles_;
  };
}
