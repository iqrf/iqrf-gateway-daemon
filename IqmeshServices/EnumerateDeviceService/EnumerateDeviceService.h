#pragma once

#include "IEnumerateDeviceService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "ITraceService.h"
#include "IJsCacheService.h"
#include <string>


namespace iqrf {

  /// \class EnumerateDeviceService
  /// \brief Implementation of IEnumerateDeviceService
  class EnumerateDeviceService : public IEnumerateDeviceService
  {
  public:
    /// \brief Constructor
    EnumerateDeviceService();

    /// \brief Destructor
    virtual ~EnumerateDeviceService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(IMessagingSplitterService* iface);
    void detachInterface(IMessagingSplitterService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

    void attachInterface(iqrf::IJsCacheService* iface);
    void detachInterface(iqrf::IJsCacheService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
