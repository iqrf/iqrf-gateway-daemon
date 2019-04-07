#pragma once

#include "IWriteTrConfService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "ITraceService.h"
#include <string>

/// Forward declaration of DpaMessage
class DpaMessage;

namespace iqrf {

  /// \class IWriteTrConfService
  /// \brief Implementation of IWriteTrConfService
  class WriteTrConfService : public IWriteTrConfService
  {
  public:
    /// \brief Constructor
    WriteTrConfService();

    /// \brief Destructor
    virtual ~WriteTrConfService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

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
