#pragma once

#include "Sensor.h"
#include "BinaryOutput.h"
#include <map>

namespace iqrf {

  class IIqrfInfo
  {
  public:
    virtual std::map<int, sensor::EnumeratePtr> getSensors() const = 0;
    virtual std::map<int, binaryoutput::EnumeratePtr> getBinaryOutputs() const = 0;
    virtual void startEnumeration() = 0;

    virtual ~IIqrfInfo() {}
  };
}
