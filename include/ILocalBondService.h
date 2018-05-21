#pragma once

#include "ShapeDefines.h"

#ifdef ILocalBondService_EXPORTS
#define ILocalBondService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define ILocalBondService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class ILocalBondService
  class ILocalBondService_DECLSPEC ILocalBondService
  {
  public:
    virtual ~ILocalBondService() {}
  };
}
