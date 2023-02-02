/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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

#include "IIqrfRestore.h"
#include "ShapeProperties.h"
#include "IMessagingSplitterService.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include "IJsRenderService.h"
#include "IJsCacheService.h"
#include "ILaunchService.h"
#include "ITraceService.h"

#include <string>

namespace iqrf {

  class IqrfRestore : public IIqrfRestore
  {
  public:
    IqrfRestore();
    virtual ~IqrfRestore();

    void restore(const uint16_t deviceAddress, std::basic_string<uint8_t>& backupData, const bool restartCoordinator) override;
    void getTransResults(std::list<std::unique_ptr<IDpaTransactionResult2>>& transResult) override;
    std::basic_string<uint16_t> getBondedNodes(void) override;
    int getErrorCode(void) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(IIqrfDpaService* iface);
    void detachInterface(IIqrfDpaService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
