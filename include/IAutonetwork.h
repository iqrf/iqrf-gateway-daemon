#pragma once

#include "ShapeDefines.h"

#ifdef IAutonetwork_EXPORTS
#define IAutonetwork_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IAutonetwork_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class IAutonetwork
  class IAutonetwork_DECLSPEC IAutonetwork
  {
  public:
    virtual ~IAutonetwork() {}
  };
}
