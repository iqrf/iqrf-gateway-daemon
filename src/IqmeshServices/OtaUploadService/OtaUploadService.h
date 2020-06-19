#pragma once

#include "IOtaUploadService.h"
#include "ILaunchService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "ITraceService.h"
#include <string>


namespace iqrf {

  /// \class OtaUploadService
  /// \brief Implementation of IOtaUploadService
  class OtaUploadService : public IOtaUploadService
  {
  public:
    /// \brief Constructor
    OtaUploadService();

    /// \brief Destructor
    virtual ~OtaUploadService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ILaunchService* iface);
    void detachInterface(shape::ILaunchService* iface);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(IMessagingSplitterService* iface);
    void detachInterface(IMessagingSplitterService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
