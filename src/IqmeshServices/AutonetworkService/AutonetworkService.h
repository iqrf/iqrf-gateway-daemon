#pragma once

#include "IAutonetworkService.h"
#include "ShapeProperties.h"
#include "IMessagingSplitterService.h"
#include "IJsCacheService.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include <string>


namespace iqrf {

  /// \class AutonetworkService
  /// \brief Implementation of IAutonetworkService
  class AutonetworkService : public IAutonetworkService
  {
  public:
    /// \brief Constructor
    AutonetworkService();

    /// \brief Destructor
    virtual ~AutonetworkService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(iqrf::IJsCacheService* iface);
    void detachInterface(iqrf::IJsCacheService* iface);

    void attachInterface(iqrf::IMessagingSplitterService* iface);
    void detachInterface(iqrf::IMessagingSplitterService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
