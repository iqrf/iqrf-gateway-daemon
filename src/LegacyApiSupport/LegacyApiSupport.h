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

#include "ShapeProperties.h"
#include "IMessagingSplitterService.h"
#include "JsonSerializer.h"
#include "IIqrfDpaService.h"
#include "ISchedulerService.h"
#include "ITraceService.h"
#include <string>

class DpaMessage;

namespace iqrf {

  class LegacyApiSupport
  {
  public:
    LegacyApiSupport();
    virtual ~LegacyApiSupport();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IMessagingSplitterService* iface);
    void detachInterface(iqrf::IMessagingSplitterService* iface);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(iqrf::ISchedulerService* iface);
    void detachInterface(iqrf::ISchedulerService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    void handleMsgFromMessaging(const std::string & messagingId, const std::basic_string<uint8_t> & msg);
    void handleAsyncDpaMessage(const DpaMessage& dpaMessage);

    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    JsonSerializer m_serializer;
    iqrf::IIqrfDpaService* m_dpa = nullptr;
    iqrf::ISchedulerService* m_scheduler = nullptr;
    std::string m_name;
    bool m_asyncDpaMessage = false;
    std::vector<std::string> m_filters =
    {
      "dpaV1"
    };

  };
}
