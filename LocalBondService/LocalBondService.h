#pragma once

#include "ILocalBondService.h"
#include "ShapeProperties.h"
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

    /// \throw std::exception if some error occured during operation
    /// \details
    /// Throws exception, if one of these situations occurs:
    /// - nodeAddr is outside of the range of [0..0xEF]
    /// - some error occured during processing of request, including DPA request processing 
    BondResult bondNode(const uint16_t nodeAddr) override;


    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:

    // checkers of service parameters
    void checkNodeAddr(const uint16_t nodeAddr);

    BondResult _bondNode(const uint16_t nodeAddr);
    bool pingNode(const uint16_t nodeAddr);
    void removeBondedNode(const uint16_t nodeAddr);


    iqrf::IIqrfDpaService* m_dpa = nullptr;
    std::string m_name;
    //IMessaging* m_messaging;
    //IDaemon* m_daemon;
    //std::vector<ISerializer*> m_serializerVect;

  };
}
