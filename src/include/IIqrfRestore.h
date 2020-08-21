#pragma once

#include "ShapeDefines.h"
#include <map>
#include <vector>
#include <list>
#include <cmath>
#include <thread>
#include <bitset>
#include <chrono>
#include "IDpaTransactionResult2.h"

#ifdef IIqrfRestore_EXPORTS
#define IIqrfRestore_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IIqrfRestore_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IIqrfRestore_DECLSPEC IIqrfRestore
  {
  public:
    virtual ~IIqrfRestore() {}
    virtual void restore(const uint16_t deviceAddress, const uint16_t dpaVersion, std::basic_string<uint8_t>& backupData, const bool restartCoordinator) = 0;
    virtual void getTransResults(std::list<std::unique_ptr<IDpaTransactionResult2>>& transResult) = 0;
    virtual std::basic_string<uint16_t> getBondedNodes(void) = 0;
  };
}
