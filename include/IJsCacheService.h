#pragma once

#include "ShapeDefines.h"
#include <vector>
#include <string>
#include <functional>

#ifdef IJsCacheService_EXPORTS
#define IJsCacheService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IJsCacheService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IJsCacheService_DECLSPEC IJsCacheService
  {
  public:
    typedef std::function<void(int statusCode, const std::string & data)> DataHandlerFunc;

    virtual const std::string& getDriver(int id) const = 0;

    virtual ~IJsCacheService() {};
  };
}
