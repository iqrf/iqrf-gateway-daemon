/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
