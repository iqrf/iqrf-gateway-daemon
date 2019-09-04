#pragma once

#include "INativeUploadService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "IMessagingSplitterService.h"
#include "IIqrfChannelService.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {

  /// \class NativeUploadService
  /// \brief Implementation of INativeUploadService
  class NativeUploadService : public INativeUploadService
  {
  public:
    /// \brief Constructor
    NativeUploadService();

    /// \brief Destructor
    virtual ~NativeUploadService();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(IMessagingSplitterService* iface);
    void detachInterface(IMessagingSplitterService* iface);

    void attachInterface(IIqrfChannelService* iface);
    void detachInterface(IIqrfChannelService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
