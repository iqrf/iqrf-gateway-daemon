#pragma once

#include "IJsCacheService.h"
#include <string>

#ifdef IJsRenderService_EXPORTS
#define IJsRenderService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IJsRenderService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IJsRenderService_DECLSPEC IJsRenderService
  {
  public:
    virtual void loadJsCodeFenced(int contextId, const std::string& js) = 0;
    virtual void mapNadrToFenced(int nadr, int contextId) = 0;
    virtual void callFenced(int nadr, const std::string& functionName, const std::string& par, std::string& ret) = 0;
    virtual ~IJsRenderService() {}
  };
}
