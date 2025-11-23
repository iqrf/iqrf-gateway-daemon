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

#include "ILaunchService.h"
#include "IApiTokenService.h"
#include "ITraceService.h"
#include "ShapeProperties.h"

namespace iqrf {

  class ApiTokenService : public IApiTokenService {
  public:
    /**
     * Constructor
     */
    ApiTokenService();

    /**
     * Destructor
     */
    virtual ~ApiTokenService();

    /**
     * Initializes component
     * @param props Component properties
     */
    void activate(const shape::Properties *props = 0);

    /**
     * Modifies component properties
     * @param props Component properties
     */
    void modify(const shape::Properties *props);

    /**
     * Deactivates component
     */
    void deactivate();

    /**
     * Return API token entity by ID
     * @param id API token ID
     * @return API token entity
     */
    std::unique_ptr<ApiToken> getApiToken(const uint32_t id) override;

    /**
     * Attaches launcher service interface
     * @param iface Launcher service interface
     */
    void attachInterface(shape::ILaunchService *iface);

    /**
     * Detaches launcher service interface
     * @param iface Launcher service interface
     */
    void detachInterface(shape::ILaunchService *iface);

    /**
     * Attaches tracing service interface
     * @param iface Tracing service interface
     */
    void attachInterface(shape::ITraceService* iface);

    /**
     * Detaches tracing service interface
     * @param iface Tracing service interface
     */
    void detachInterface(shape::ITraceService* iface);
  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
  };
}
