#pragma once

#include "IJsCacheService.h"
#include "Trace.h"
#include <string>

namespace iqrf {
  class IJsRenderService
  {
  public:
    // used to map provisionalDrivers load per HWPID
    static const int HWPID_DEFAULT_MAPPING = -0x10000;
    static const int HWPID_MAPPING_SPACE = -0x20000;

    virtual void loadJsCodeFenced(int contextId, const std::string& js) = 0;
    virtual void mapNadrToFenced(int nadr, int contextId) = 0;
    virtual void callFenced(int nadr, int hwpid, const std::string& functionName, const std::string& par, std::string& ret) = 0;
    virtual void unloadProvisionalContexts() = 0;
    virtual ~IJsRenderService() {}
  };
}
