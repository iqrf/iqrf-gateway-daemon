#pragma once

#include "ShapeDefines.h"
#include <string>

#ifdef IBaseService_EXPORTS
#define IBaseService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IBaseService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IBaseService_DECLSPEC IBaseService
  {
  public:
    virtual ~IBaseService() {}
  };
}
