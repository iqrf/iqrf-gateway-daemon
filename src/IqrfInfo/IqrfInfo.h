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

#include "TimeConversion.h"
#include "ShapeProperties.h"
#include "IIqrfInfo.h"
#include "IJsRenderService.h"
#include "IJsCacheService.h"
#include "IIqrfDpaService.h"
#include "ILaunchService.h"
#include "ITraceService.h"

#include <filesystem>

namespace iqrf {
  class IqrfInfo : public IIqrfInfo
  {
  public:
    IqrfInfo();
    virtual ~IqrfInfo();

    std::map<int, sensor::EnumeratePtr> getSensors() const override;
    std::map<int, binaryoutput::EnumeratePtr> getBinaryOutputs() const override;
    std::vector<int> getLights() const override;
    std::map<int, embed::node::BriefInfoPtr> getNodes() const override;
    std::map<uint8_t, embed::node::NodeMidHwpid> getNodeMidHwpidMap() const override;

    void insertNodes(const std::map<int, embed::node::BriefInfo> & nodes) override;

    // start enumeration thread
    void startEnumeration() override;
    // stop enumeration thread
    void stopEnumeration() override;
    // stop enumerate now (must be started to take effect)
    void enumerate() override;
    // get enumeration period
    int getPeriodEnumerate() const override;
    // set enumeration period (must be started to take periodic effect)
    void setPeriodEnumerate(int period) override;
    // get non bonded nodes
    std::vector<uint32_t> getUnbondMids() const override;
    // remove unbond nodes from DB - nodes are not by default deleted if unbonded
    void removeUnbondMids(const std::vector<uint32_t> & unbondVec) override;

    void registerEnumerateHandler(const std::string& clientId, EnumerateHandlerFunc fun) override;
    void unregisterEnumerateHandler(const std::string& clientId) override;

    bool getMidMetaDataToMessages() const override;
    void setMidMetaDataToMessages(bool val) override;

    rapidjson::Document getMidMetaData(uint32_t mid) const override;
    void setMidMetaData(uint32_t mid, const rapidjson::Value & metaData) override;

    rapidjson::Document getNodeMetaData(int nadr) const override;
    void setNodeMetaData(int nadr, const rapidjson::Value & metaData) override;

    // resets database
    void resetDb() override;
    void reloadDrivers() override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IJsRenderService* iface);
    void detachInterface(iqrf::IJsRenderService* iface);

    void attachInterface(iqrf::IJsCacheService* iface);
    void detachInterface(iqrf::IJsCacheService* iface);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
