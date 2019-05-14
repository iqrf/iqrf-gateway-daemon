#pragma once

#include "IJsDriverService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include "IJsRenderService.h"
#include <string>


namespace iqrf {

  class JsDriverService : public IJsDriverService
  {
  public:
    JsDriverService();

    /// \brief Destructor
    virtual ~JsDriverService();

    DpaMessage createDpaRequest(JsDriverRequest & jsRequest) override;
    void processDpaTransactionResult(JsDriverRequest & jsDriverRequest, std::unique_ptr<IDpaTransactionResult2> res) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

    void attachInterface(iqrf::IJsRenderService* iface);
    void detachInterface(iqrf::IJsRenderService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
