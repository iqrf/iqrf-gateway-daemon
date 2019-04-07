#pragma once

#include "ShapeDefines.h"

#ifdef IAutonetworkService_EXPORTS
#define IAutonetworkService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IAutonetworkService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class IAutonetworkService
  class IAutonetworkService_DECLSPEC IAutonetworkService
  {
  public:
    virtual ~IAutonetworkService() {}
  };
}
