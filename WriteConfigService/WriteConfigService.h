#pragma once

#include "IWriteConfigService.h"
#include "ShapeProperties.h"
#include "IIqrfDpaService.h"
#include "ITraceService.h"
#include <string>

/// Forward declaration of DpaMessage
class DpaMessage;

namespace iqrf {

  /// \class IWriteConfigService
  /// \brief Implementation of IWriteConfigService
  class WriteConfigService : public IWriteConfigService
  {
  public:
    /// \brief Constructor
    WriteConfigService();

    /// \brief Destructor
    virtual ~WriteConfigService();

    /// \throw std::exception if some error occured during operation
    /// \details
    /// Throws exception, if one of these situations occurs:
    /// - some addresses of configuration bytes are outside of the range of: [0 .. 0xEF]
    /// - there are 2 or more config bytes with the same address
    /// - list of target nodes is empty
    /// - some addresses from target nodes are outside of the range of: [0x01 .. 0x20]
    /// - config bytes contain RF channel bytes, which are not in accordance with
    ///   coordinator RF channel band
    /// - some error occured during operation of write
    WriteResult writeConfigBytes(
      const std::vector<HWP_ConfigByte>& configBytes,
      const std::list<uint16_t>& targetNodes
    ) override;


    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(iqrf::IIqrfDpaService* iface);
    void detachInterface(iqrf::IIqrfDpaService* iface);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:

    // checkers of service parameters
    void checkTargetNodes(const std::list<uint16_t>& targetNodes);
    void checkConfigBytes(const std::vector<HWP_ConfigByte>& configBytes);

    std::list<uint16_t> getBondedNodes();

    void writeHwpConfiguration(
      WriteResult& writeResult,
      const std::vector<HWP_ConfigByte>& configBytes,
      const uint16_t nodeAddr
    );

    void _writeConfigBytes(
      WriteResult& writeResult,
      const std::vector<HWP_ConfigByte>& configBytes, 
      const std::list<uint16_t>& targetNodes
    );
    
    void updateCoordRfChannelBand();
    void checkRfChannelIfPresent(const std::vector<HWP_ConfigByte>& configBytes);

    // Coordinator's RF channel band
    uint8_t m_coordRfChannelBand;

    iqrf::IIqrfDpaService* m_dpa = nullptr;
    std::string m_name;
    //IMessaging* m_messaging;
    //IDaemon* m_daemon;
    //std::vector<ISerializer*> m_serializerVect;

  };
}
