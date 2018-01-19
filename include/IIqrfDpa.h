#pragma once

#include "ShapeDefines.h"
#include <string>

#ifdef IIqrfDpa_EXPORTS
#define IIqrfDpa_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IIqrfDpa_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrfgw {
  class IIqrfDpa_DECLSPEC IIqrfDpa
  {
  public:
    virtual ~IIqrfDpa() {}
  };
}
