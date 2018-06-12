#pragma once

#include "ShapeDefines.h"

#ifdef IEnumerateDeviceService_EXPORTS
#define IEnumerateDeviceService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IEnumerateDeviceService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class IEnumerateDeviceService
  class IEnumerateDeviceService_DECLSPEC IEnumerateDeviceService
  {
  public:
    virtual ~IEnumerateDeviceService() {}
  };
}
