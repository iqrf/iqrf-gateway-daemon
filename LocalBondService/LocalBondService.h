#pragma once

#include "ILocalBondService.h"
#include "ShapeProperties.h"
#include "IMessagingSplitterService.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include <string>


namespace iqrf {

  /// \class LocalBondService
  /// \brief Implementation of ILocalBondService
  class LocalBondService : public ILocalBondService
  {
  public:
    /// \brief Constructor
    LocalBondService();

    /// \brief Destructor
    virtual ~LocalBondService();

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
