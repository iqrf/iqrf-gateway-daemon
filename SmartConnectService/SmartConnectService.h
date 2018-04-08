#pragma once

#include "ISmartConnectService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include <string>

/// Forward declaration of DpaMessage
class DpaMessage;

namespace iqrf {

  /// \class SmartConnectService
  /// \brief Implementation of ISmartConnectService
  class SmartConnectService : public ISmartConnectService
  {
  public:
    /// \brief Constructor
    SmartConnectService();

    /// \brief Destructor
    virtual ~SmartConnectService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
