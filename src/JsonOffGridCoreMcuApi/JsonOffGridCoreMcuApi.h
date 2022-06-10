/**
 * Copyright 2020 MICRORISC,s.r.o.
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

#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "IOffGridCoreMcu.h"
#include "IMessagingSplitterService.h"
#include "IConfigurationService.h"
#include "ITraceService.h"
#include <string>
#include <thread>

namespace iqrf {
  class JsonOffGridCoreMcuApi
  {
  public:
    JsonOffGridCoreMcuApi();
    virtual ~JsonOffGridCoreMcuApi();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(iqrf::IOffGridCoreMcu* iface);
    void detachInterface(iqrf::IOffGridCoreMcu* iface);

    void attachInterface(iqrf::IMessagingSplitterService* iface);
    void detachInterface(iqrf::IMessagingSplitterService* iface);

    void attachInterface(shape::IConfigurationService* iface);
    void detachInterface(shape::IConfigurationService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
