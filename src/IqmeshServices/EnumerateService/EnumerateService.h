#pragma once

#include "IEnumerateService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "IJsCacheService.h"
#include "IJsDriverService.h"
#include "ITraceService.h"
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

    IFastEnumerationPtr getFastEnumeration() const override;
    INodeDataPtr getNodeData(uint16_t nadr) const override;
    IStandardSensorDataPtr getStandardSensorData(uint16_t nadr) const override;
    IStandardBinaryOutputDataPtr getStandardBinaryOutputData(uint16_t nadr) const override;
    embed::explore::PeripheralInformationPtr getPeripheralInformationData(uint16_t nadr, int per) const override;


    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(iqrf::IJsCacheService* iface);
    void detachInterface(iqrf::IJsCacheService* iface);

    void attachInterface(iqrf::IJsDriverService* iface);
    void detachInterface(iqrf::IJsDriverService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);
  
  private:
    class Imp;
    Imp* m_imp;
  };
}
