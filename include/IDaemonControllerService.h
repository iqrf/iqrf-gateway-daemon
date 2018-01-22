#pragma once

#include "ShapeDefines.h"
#include <string>

#ifdef IDaemonControllerService_EXPORTS
#define IDaemonControllerService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IDaemonControllerService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IDaemonControllerService_DECLSPEC IDaemonControllerService
  {
  public:
    virtual std::string doService(const std::string & str) const = 0;
    virtual ~IDaemonControllerService() {}
  };
}
