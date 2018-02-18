#pragma once

#include "ShapeProperties.h"
#include "Ide4Counterpart.h"
#include "ITraceService.h"
#include "IMessagingService.h"
#include "IIqrfChannelService.h"
#include "DpaHandler.h"
#include "TaskQueue.h"
#include <string>

namespace iqrf {
  class Ide4Counterpart
  {
  public:
    /// \brief operational mode
    /// \details
    /// Operational is used for normal work
    /// Service the only UDP Messaging is used to communicate with IQRF IDE
    /// Forwarding normal work but all DPA messages are forwarded to IQRF IDE to me monitored there
    enum class Mode {
      Operational,
      Service,
      Forwarding
    };

    Ide4Counterpart();
    virtual ~Ide4Counterpart();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

    void attachInterface(iqrf::IMessagingService* iface);
    void detachInterface(iqrf::IMessagingService* iface);

    void attachInterface(iqrf::IIqrfChannelService* iface);
    void detachInterface(iqrf::IIqrfChannelService* iface);

    /// \brief switch operational mode
    /// \param [in] mode operational mode to switch
    /// \details
    /// \details
    /// Operational is used for normal work
    /// Service the only UDP Messaging is used to communicate with IQRF IDE
    /// Forwarding normal work but all DPA messages are forwarded to IQRF IDE to me monitored there
    void setMode(Mode mode);

  private:
    /// \brief Get GW identification for IQRF IDE
    /// \param [out] message composed message to send
    void getGwIdent(std::basic_string<unsigned char>& message);

    /// \brief Get GW status for IQRF IDE
    /// \param [out] message composed message to send
    void getGwStatus(std::basic_string<unsigned char>& message);

    /// \brief UDP message handler
    int handleMessageFromUdp(const std::basic_string<unsigned char>& udpMessage);

    /// \brief encode message
    /// \param [out] udpMessage encoded message
    /// \param [in] message to insert to udpMessage
    void encodeMessageUdp(std::basic_string<unsigned char>& udpMessage, const std::basic_string<unsigned char>& message = std::basic_string<unsigned char>());

    /// \brief decode message
    /// \param [in] udpMessage to decode
    /// \param [out] message content of udpMessage
    void decodeMessageUdp(const std::basic_string<unsigned char>& udpMessage, std::basic_string<unsigned char>& message);

  private:
    IMessagingService *m_messaging = nullptr;
    IIqrfChannelService *m_iqrfChannelService = nullptr;
    TaskQueue<DpaTransaction*> *m_dpaTransactionQueue = nullptr;
    std::mutex m_modeMtx;
    Mode m_mode;
    std::unique_ptr<IIqrfChannelService::Accessor> m_exclusiveAcessor;
    std::unique_ptr<IIqrfChannelService::Accessor> m_snifferAcessor;
    int sendMessageToIde(const std::basic_string<unsigned char>& message);
  };
}
