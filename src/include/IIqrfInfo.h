#pragma once

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
    virtual void startEnumeration() = 0;
    virtual rapidjson::Document getNodeMetaData(int nadr) const = 0;
    virtual void setNodeMetaData(int nadr, const rapidjson::Value & metaData) = 0;

    virtual ~IIqrfInfo() {}
  };
}
