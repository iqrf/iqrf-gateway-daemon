/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#include <stdexcept>
#include <type_traits>

namespace iqrf {

  /// Enum utility class
  class EnumUtils {
  public:

    /**
     * Return scalar value from enum member
     * @tparam T Enum type
     * @param e Enum member
     * @return std::underlying_type_t<T> Member value of underlying type
     */
    template<typename T>
    static auto toScalar(T e) -> typename std::underlying_type_t<T> {
      if (!std::is_enum_v<T>) {
        throw std::domain_error("Passed parameter is not enumeration type.");
      }
      return static_cast<std::underlying_type_t<T>>(e);
    }
  };
}
