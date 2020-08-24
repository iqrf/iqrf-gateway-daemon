#pragma once

#include "IBackupService.h"
#include "IIqrfBackup.h"
#include "ShapeProperties.h"
#include "IMessagingSplitterService.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {

  class BackupService: public IBackupService
  {
  public:
    BackupService();
    virtual ~BackupService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(IIqrfBackup* iface);
    void detachInterface(IIqrfBackup* iface);

    void attachInterface(IMessagingSplitterService* iface);
    void detachInterface(IMessagingSplitterService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
