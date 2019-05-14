#pragma once

#include "JsDriverRequest.h"
#include "IDpaTransactionResult2.h"

namespace iqrf {

  class IJsDriverService
  {
  public:
    virtual DpaMessage createDpaRequest(JsDriverRequest & jsRequest) = 0;
    virtual void processDpaTransactionResult(JsDriverRequest & jsDriverRequest, std::unique_ptr<IDpaTransactionResult2> res) = 0;
    virtual ~IJsDriverService() {}
  };
}
