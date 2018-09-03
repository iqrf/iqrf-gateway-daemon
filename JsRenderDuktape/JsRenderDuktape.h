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

    void loadJsCode(const std::string& js) override;
    void call(const std::string& functionName, const std::string& par, std::string& ret) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(IJsCacheService* iface);
    void detachInterface(IJsCacheService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
