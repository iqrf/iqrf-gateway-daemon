#pragma once

#include "IEnumerateService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include "IJsDriverService.h"
#include <string>


namespace iqrf {

  /// \class EnumerateService
  /// \brief Implementation of IEnumerateService
  class EnumerateService : public IEnumerateService
  {
  public:
    /// \brief Constructor
    EnumerateService();

    /// \brief Destructor
    virtual ~EnumerateService();

    CoordinatorData getCoordinatorData() const override;
    NodeData getNodeData(uint16_t nadr) const override;
    IStandardSensorDataPtr getStandardSensorData(uint16_t nadr) const override;
    IStandardBinaryOutputDataPtr getStandardBinaryOutputData(uint16_t nadr) const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

    void attachInterface(iqrf::IJsDriverService* iface);
    void detachInterface(iqrf::IJsDriverService* iface);

  private:
    class Imp;
    Imp* m_imp;
  };
}
