#pragma once

#include "IJsRenderService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <map>

namespace iqrf {
  class JsRenderDuktape : public IJsRenderService
  {
  public:
    JsRenderDuktape();
    virtual ~JsRenderDuktape();

    void loadJsCodeFenced(int contextId, const std::string& js) override;
    // set driver mapping according nadr
    void mapNadrToFenced(int nadr, int contextId) override;
    void callFenced(int nadr, int hwpid, const std::string& functionName, const std::string& par, std::string& ret) override;
    void unloadProvisionalContexts() override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
