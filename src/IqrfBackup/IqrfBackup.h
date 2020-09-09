#pragma once

#include "IIqrfBackup.h"
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

  class IqrfBackup: public IIqrfBackup
  {
  public:
    IqrfBackup();
    virtual ~IqrfBackup();
    
    void backup(const uint16_t address, DeviceBackupData& backupData) override;
    void getTransResults(std::list<std::unique_ptr<IDpaTransactionResult2>>& transResult) override;
    std::basic_string<uint16_t> getBondedNodes(void) override;

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
