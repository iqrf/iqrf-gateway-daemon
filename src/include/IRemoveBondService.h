#pragma once

#include "ShapeDefines.h"

#ifdef IRemoveBondService_EXPORTS
#define IRemoveBondService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IRemoveBondService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class IRemoveBondService
  class IRemoveBondService_DECLSPEC IRemoveBondService
  {
  public:
    virtual ~IRemoveBondService() {}
  };
}
