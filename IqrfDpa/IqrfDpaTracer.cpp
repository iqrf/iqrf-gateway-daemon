/**
 * Copyright 2016-2017 MICRORISC s.r.o.
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

#include "IqrfTrace.h"
#include "StaticTracer.h"

namespace iqrf {

  /////////////////////////////////////////
  void tracerStart(const std::string& filename, TrcLevel level, int filesize)
  {
    //not required
  }

  void tracerStop()
  {
    //not required
  }

  bool tracerIsValid(TrcLevel level, int channel)
  {
    return shape::Tracer::get().isValid((int)level, channel);
  }

  void tracerMessage(TrcLevel level, int channel, const char* moduleName,
    const char* sourceFile, int sourceLine, const char* funcName, const std::string & msg)
  {
    shape::Tracer::get().writeMsg((int)level, channel, moduleName, sourceFile, sourceLine, funcName, msg);
  }
}
