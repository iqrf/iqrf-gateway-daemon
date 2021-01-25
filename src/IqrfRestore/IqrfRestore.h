#pragma once

#include "IIqrfRestore.h"
#include "ShapeProperties.h"
#include "IMessagingSplitterService.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include "IJsRenderService.h"
#include "IJsCacheService.h"
#include "ILaunchService.h"
#include "ITraceService.h"

#include <string>

namespace iqrf {

  class IqrfRestore : public IIqrfRestore
  {
  public:
    IqrfRestore();
    virtual ~IqrfRestore();

    void restore(const uint16_t deviceAddress, std::basic_string<uint8_t>& backupData, const bool restartCoordinator) override;
    void getTransResults(std::list<std::unique_ptr<IDpaTransactionResult2>>& transResult) override;
    std::basic_string<uint16_t> getBondedNodes(void) override;
    int getErrorCode(void) override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(IIqrfDpaService* iface);
    void detachInterface(IIqrfDpaService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
