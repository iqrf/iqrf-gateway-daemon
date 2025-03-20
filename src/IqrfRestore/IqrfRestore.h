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
