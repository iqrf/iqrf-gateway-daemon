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

#include "IIqrfDpaService.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "Sensor.h"
#include "BinaryOutput.h"
#include "Dali.h"
#include "Light.h"
#include "EmbedNode.h"
#include <map>

namespace iqrf
{
  class IIqrfInfo
  {
  public:
    virtual std::map<int, sensor::EnumeratePtr> getSensors() const = 0;
    virtual std::map<int, binaryoutput::EnumeratePtr> getBinaryOutputs() const = 0;
    virtual std::map<int, dali::EnumeratePtr> getDalis() const = 0;
    virtual std::map<int, light::EnumeratePtr> getLights() const = 0;
    virtual std::map<int, embed::node::BriefInfoPtr> getNodes() const = 0;

    // for AutoNetwork usage
    virtual void insertNodes(const std::map<int, embed::node::BriefInfo> & nodes) = 0;

    // start enumeration thread
    virtual void startEnumeration() = 0;
    // stop enumeration thread
    virtual void stopEnumeration() = 0;
    // stop enumerate now (must be started to take effect)
    virtual void enumerate() = 0;
    // get enumeration period
    virtual int getPeriodEnumerate() const = 0;
    // set enumeration period (must be started to take periodic effect)
    virtual void setPeriodEnumerate(int period) = 0;
    // get non bonded nodes
    virtual std::vector<uint32_t> getUnbondMids() const = 0;
    // remove unbond nodes from DB - nodes are not by default deleted if unbonded
    virtual void removeUnbondMids(const std::vector<uint32_t> & unbondVec) = 0;

    class EnumerationState {
    public:
      enum class Phase
      {
        start,
        check,
        fullNode,
        fullDevice,
        standard,
        finish
      };

      EnumerationState()
      {}

      EnumerationState(Phase phase, int step, int steps)
        :m_phase(phase)
        ,m_step(step)
        ,m_steps(steps)
        ,m_percentage(0)
      {}

      Phase m_phase = Phase::start;
      int m_step = 1; // step order in steps
      int m_steps = 1; // number of steps of the phase
      int m_percentage = 0; // rough percetage estimation of overall enumeration procedure
    };

    typedef std::function<void(EnumerationState es)> EnumerateHandlerFunc;
    virtual void registerEnumerateHandler(const std::string& clientId, EnumerateHandlerFunc fun) = 0;
    virtual void unregisterEnumerateHandler(const std::string& clientId) = 0;

    // for Mid meta data
    // gets the flag to control if messages are anotaded by metadata
    virtual bool getMidMetaDataToMessages() const = 0;
    // sets the flag to control if messages are anotaded by metadata
    virtual void setMidMetaDataToMessages(bool val) = 0;

    virtual rapidjson::Document getMidMetaData(uint32_t mid) const = 0;
    virtual void setMidMetaData(uint32_t mid, const rapidjson::Value & metaData) = 0;

    virtual rapidjson::Document getNodeMetaData(int nadr) const = 0;
    virtual void setNodeMetaData(int nadr, const rapidjson::Value & metaData) = 0;

    virtual void resetDb() = 0;
    virtual void reloadDrivers() = 0;

    virtual ~IIqrfInfo() {}
  };
}
