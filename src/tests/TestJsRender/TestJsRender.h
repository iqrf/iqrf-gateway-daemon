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

#include "IJsRenderService.h"
#include "ShapeProperties.h"
#include "ILaunchService.h"
#include "ITraceService.h"

namespace iqrf {

  class TestJsRender
  {
  public:
    TestJsRender();
    virtual ~TestJsRender();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IJsRenderService* iface);
    void detachInterface(iqrf::IJsRenderService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  };

}
