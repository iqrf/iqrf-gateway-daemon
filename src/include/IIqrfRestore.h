/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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
