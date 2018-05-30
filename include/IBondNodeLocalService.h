#pragma once

#include "ShapeDefines.h"

#ifdef IBondNodeLocalService_EXPORTS
#define IBondNodeLocalService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IBondNodeLocalService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {

  /// \class IBondNodeLocalService
  class IBondNodeLocalService_DECLSPEC IBondNodeLocalService
  {
  public:
    virtual ~IBondNodeLocalService() {}
  };
}
