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
    virtual void loadJsCodeFenced(int id, const std::string& js) = 0;
    virtual void callFenced(int id, const std::string& functionName, const std::string& par, std::string& ret) = 0;
    virtual void loadJsCode(const std::string& js) = 0;
    virtual void call(const std::string& functionName, const std::string& par, std::string& ret) = 0;
    virtual ~IJsRenderService() {}
  };
}
