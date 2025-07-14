/*
* filename: IqrfTcp.cpp
* author: Karel Hanák <xhanak34@stud.fit.vutbr.cz>
* school: Brno University of Technology, Faculty of Information Technology
* bachelor's thesis: Automatic Testing of Software
*
* This file contains implementation a TCP communication component in the role of a client.
*
* Copyright 2020 Karel Hanák
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#define IIqrfChannelService_EXPORTS

#include "IqrfTcp.h"
#include "AccessControl.h"
#include "rapidjson/pointer.h"
#include <thread>
#include <atomic>
#include <cstring>

#include "iqrf/connector/tcp/TcpConnector.h"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrf__IqrfTcp.hxx"

TRC_INIT_MODULE(iqrf::IqrfTcp)

const unsigned BUFFER_SIZE = 1024;

namespace iqrf {

  class IqrfTcp::Imp {
   public:
    Imp(): m_accessControl(this) {}

    ~Imp() {}

    /**
     * Sends DPA message to TCP server via the socket file descriptor.
     *
     * @param message DPA message to send
     */
    void send(const std::basic_string<unsigned char> &message) {
      m_sendBuffer.assign(message.begin(), message.end());
      TRC_INFORMATION("Sending to IQRF TCP: " << std::endl << MEM_HEX(message.data(), message.size()));
      try {
        m_connector->send(m_sendBuffer);
        m_accessControl.sniff(message);
      } catch (const std::exception &e) {
        TRC_WARNING("Failed to send data via connector: " << e.what());
        throw;
      }
    }

    bool enterProgrammingState() {
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("Not implemented");
      TRC_FUNCTION_LEAVE("");
      return false;
    }

    IIqrfChannelService::UploadErrorCode upload(
      const UploadTarget target,
      const std::basic_string <uint8_t> &data,
      const uint16_t address
    ) {
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("Not implemented");
      //silence -Wunused-parameter
      (void) target;
      (void) data;
      (void) address;

      TRC_FUNCTION_LEAVE("");
      //return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;
      return IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
    }

    bool terminateProgrammingState() {
      TRC_INFORMATION("Terminating programming mode.");
      TRC_WARNING("Not implemented");
      return false;
    }

    void startListen() {
      m_runListenThread = true;
      m_listenThread = std::thread(&IqrfTcp::Imp::listen, this);
    }

    IIqrfChannelService::State getState() const {
      IIqrfChannelService::State state = State::NotReady;
      if (m_accessControl.hasExclusiveAccess())
        state = State::ExclusiveAccess;
      else if (m_runListenThread)
        state = m_connector->getState() == iqrf::connector::State::Ready ? State::Ready : State::NotReady;
      return state;
    }

    std::unique_ptr <IIqrfChannelService::Accessor> getAccess(ReceiveFromFunc receiveFromFunc, AccesType access) {
      return m_accessControl.getAccess(receiveFromFunc, access);
    }

    bool hasExclusiveAccess() const {
      return m_accessControl.hasExclusiveAccess();
    }

    IIqrfChannelService::osInfo getTrModuleInfo() {
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("Reading TR module identification - not implemented.");

      IIqrfChannelService::osInfo myOsInfo;
      memset(&myOsInfo, 0, sizeof(myOsInfo));

      TRC_FUNCTION_LEAVE("");
      return myOsInfo;
    }

    /**
     * Reads information from component configuration used to establish a TCP connection.
     *
     * @param props shape component configuration
     */
    void activate(const shape::Properties *props) {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfTcp instance activate" << std::endl <<
        "******************************"
      );
      m_recvBuffer.reserve(64);
      m_sendBuffer.reserve(64);
      modify(props);
    }

    /**
     * Deactivates the TCP component after closing the connection on socket file descriptor.
     */
    void deactivate() {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfTcp instance deactivate" << std::endl <<
        "******************************"
      );

      TRC_DEBUG("joining TCP listening thread");
      m_runListenThread = false;
      if (m_listenThread.joinable()) {
        m_listenThread.join();
      }
      TRC_DEBUG("listening thread joined");
      m_connector.reset();

      TRC_FUNCTION_LEAVE("")
    }

    /**
     * Modifies the TCP component configuration and recreates the TCP connector.
     *
     * @param props shape component configuration
     */
    void modify(const shape::Properties *props) {
      using namespace rapidjson;

      std::string addrStr;
      uint16_t portnum = 0;
      Document d;
      d.CopyFrom(props->getAsJson(), d.GetAllocator());

      //read target server address from configuration
      Value *address = Pointer("/address").Get(d);
      if (address != nullptr && address->IsString()) {
        addrStr = address->GetString();
      } else {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /address");
      }

      //read target port from configuration
      Value *port = Pointer("/port").Get(d);
      if (port != nullptr && port->IsUint()) {
        if (port->GetUint() == 0 || port->GetUint() > 65535) {
          THROW_EXC_TRC_WAR(std::logic_error, "Invalid port number: " + std::to_string(portnum));
        }
        portnum = port->GetUint();
      } else {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /port");
      }

      iqrf::connector::tcp::TcpConfig config(addrStr, portnum);
      m_connector = std::make_unique<iqrf::connector::tcp::TcpConnector>(config);
    }

    /**
     * The client socket listens for incoming messages from a TCP server.
     * Messages are received and handled by messageHandler.
     */
    void listen() {
      TRC_FUNCTION_ENTER("thread starts");

      try {
        while (m_runListenThread) {
          if (this->getState() == State::NotReady) {
            THROW_EXC_TRC_WAR(std::logic_error, "TCP interface not ready to listen.");
          }

          auto data = m_connector->receive();

          if (!data.empty()) {
            m_recvBuffer.assign(data.begin(), data.end());
            TRC_INFORMATION(
              "Received from IQRF TCP: " << std::endl <<
              MEM_HEX(m_recvBuffer.data(), m_recvBuffer.size())
            );
            m_accessControl.messageHandler(m_recvBuffer);
          }
        }
      } catch (const std::exception &e) {
        TRC_WARNING("Listening thread error: " << e.what());
        m_runListenThread = false;
      }
      TRC_WARNING("Listening thread stopped");
    }

   private:
    AccessControl<IqrfTcp::Imp> m_accessControl;

    std::basic_string <uint8_t> m_recvBuffer;
    std::vector <uint8_t> m_sendBuffer;

    std::atomic_bool m_runListenThread;
    std::thread m_listenThread;

    std::unique_ptr <iqrf::connector::tcp::TcpConnector> m_connector = nullptr;
  };

  //////////////////////////////////////////////////
  IqrfTcp::IqrfTcp() {
    m_imp = shape_new Imp();
  }

  IqrfTcp::~IqrfTcp() {
    delete m_imp;
  }

  void IqrfTcp::startListen() {
    return m_imp->startListen();
  }

  IIqrfChannelService::State IqrfTcp::getState() const {
    return m_imp->getState();
  }

  std::unique_ptr <IIqrfChannelService::Accessor>
  IqrfTcp::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access) {
    return m_imp->getAccess(receiveFromFunc, access);
  }

  bool IqrfTcp::hasExclusiveAccess() const {
    return m_imp->hasExclusiveAccess();
  }

  void IqrfTcp::activate(const shape::Properties *props) {
    m_imp->activate(props);
  }

  void IqrfTcp::deactivate() {
    m_imp->deactivate();
  }

  void IqrfTcp::modify(const shape::Properties *props) {
    m_imp->modify(props);
  }

  void IqrfTcp::attachInterface(shape::ITraceService *iface) {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfTcp::detachInterface(shape::ITraceService *iface) {
    shape::Tracer::get().removeTracerService(iface);
  }
}
