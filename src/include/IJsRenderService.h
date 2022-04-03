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

#include "IJsCacheService.h"
#include "Trace.h"
#include <string>
#include <set>

namespace iqrf {
  class IJsRenderService
  {
  public:
    // used to map provisionalDrivers load per HWPID
    static const int HWPID_DEFAULT_MAPPING = -0x10000;
    static const int HWPID_MAPPING_SPACE = -0x20000;

    virtual void loadJsCodeFenced(int contextId, const std::string& js, const std::set<int> & driverIdSet) = 0;
    virtual std::set<int> getDriverIdSet(int contextId) const = 0;
    virtual void mapNadrToFenced(int nadr, int contextId) = 0;
    virtual void callFenced(int nadr, int hwpid, const std::string& functionName, const std::string& par, std::string& ret) = 0;
    virtual void clearContexts() = 0;
    virtual ~IJsRenderService() {}
  };
}
