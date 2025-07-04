/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include "IIqrfChannelService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>


/** PGM Switch GPIO. */
#define PGM_SWITCH_GPIO_DEFAULT 22
/** Bus enable GPIO. */
#define BUS_ENABLE_GPIO_DEFAULT 7
/** Power enable GPIO. */
#define POWER_ENABLE_GPIO_DEFAULT 23
/** I2C enable GPIO */
#define I2C_ENABLE_GPIO_DEFAULT -1
/** SPI enable GPIO */
#define SPI_ENABLE_GPIO_DEFAULT -1
/** UART enable GPIO */
#define UART_ENABLE_GPIO_DEFAULT -1
/** Baud rate */
#define UART_BAUD_RATE_DEFAULT 57600

namespace iqrf {
  class ITraceService;

  class IqrfUart : public IIqrfChannelService
  {
  public:
    class Imp;

    IqrfUart();
    virtual ~IqrfUart();

    void startListen() override;
    State getState() const override;
    std::unique_ptr<Accessor> getAccess(ReceiveFromFunc receiveFromFunc, AccesType access) override;
    bool hasExclusiveAccess() const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    Imp* m_imp = nullptr;
  };
}
