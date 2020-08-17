#pragma once

#include "ShapeDefines.h"
#include <map>
#include <vector>

#ifdef IBackupService_EXPORTS
#define IBackupService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IBackupService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IBackupService_DECLSPEC IBackupService
  {
  public:
    virtual ~IBackupService() {}
  };
}
