#pragma once

#include "ShapeDefines.h"
#include <map>
#include <vector>

#ifdef IRestoreService_EXPORTS
#define IRestoreService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IRestoreService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IRestoreService_DECLSPEC IRestoreService
  {
  public:
    virtual ~IRestoreService() {}
  };
}
