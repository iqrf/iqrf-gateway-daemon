#pragma once

#include "IRemoveBondService.h"
#include "ShapeProperties.h"
#include "IMessagingSplitterService.h"
#include "IJsCacheService.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include <string>


namespace iqrf {

  /// \class RemoveBondService
  /// \brief Implementation of IRemoveBondService
  class RemoveBondService : public IRemoveBondService
  {
  public:
    /// \brief Constructor
    RemoveBondService();

    /// \brief Destructor
    virtual ~RemoveBondService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(iqrf::IMessagingSplitterService* iface);
    void detachInterface(iqrf::IMessagingSplitterService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
