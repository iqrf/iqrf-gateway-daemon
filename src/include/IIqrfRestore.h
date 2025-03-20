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

#include "ShapeDefines.h"
#include <map>
#include <vector>
#include <list>
#include <cmath>
#include <thread>
#include <bitset>
#include <chrono>
#include "IDpaTransactionResult2.h"

#ifdef IIqrfRestore_EXPORTS
#define IIqrfRestore_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IIqrfRestore_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IIqrfRestore_DECLSPEC IIqrfRestore
  {
  public:
    virtual ~IIqrfRestore() {}
    virtual void restore(const uint16_t deviceAddress, std::basic_string<uint8_t>& backupData, const bool restartCoordinator) = 0;
    virtual void getTransResults(std::list<std::unique_ptr<IDpaTransactionResult2>>& transResult) = 0;
    virtual std::basic_string<uint16_t> getBondedNodes(void) = 0;
    virtual int getErrorCode(void) = 0;
  };
}
