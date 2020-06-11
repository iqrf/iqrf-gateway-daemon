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
#include <mutex>
#include <regex>
#include <thread>
#include <atomic>
#include <cstring>
#ifdef SHAPE_PLATFORM_WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#ifndef SHAPE_PLATFORM_WINDOWS
#include <termios.h>
#endif

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrf__IqrfTcp.hxx"

TRC_INIT_MODULE(iqrf::IqrfTcp);

const unsigned BUFFER_SIZE = 1024;
int sockfd; //client socket file descriptor
char buffer[BUFFER_SIZE]; //buffer for incoming messages

namespace iqrf {

  class IqrfTcp::Imp
  {
  public:
    Imp()
      :m_accessControl(this)
    {
    }

    ~Imp()
    {
    }

    /**
     * Sends DPA message to TCP server via the socket file descriptor.
     * 
     * @param message DPA message to send 
     */
    void send(const std::basic_string<unsigned char>& message)
    {
      TRC_INFORMATION("Sending to IQRF TCP: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));
      
      if (sockfd == -1) {
        THROW_EXC_TRC_WAR(std::logic_error, "Socket is not open.")
      }

      if ((::send(sockfd, message.data(), message.size(), 0)) == -1) {
        TRC_WARNING("Cannot send message.");
      } else {
        TRC_INFORMATION("Message successfully sent.");
      }

    }

    bool enterProgrammingState() 
    {
      TRC_FUNCTION_ENTER("");
      //TRC_INFORMATION("Entering programming mode.");
      TRC_WARNING("Not implemented");
      TRC_FUNCTION_LEAVE("");
      //return true;
      return false;
    }

    IIqrfChannelService::UploadErrorCode upload(
      const UploadTarget target,
      const std::basic_string<uint8_t>& data,
      const uint16_t address
    )
    {
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("Not implemented");
      //silence -Wunused-parameter
      (void)target; 
      (void)data;
      (void)address;

      TRC_FUNCTION_LEAVE("");
      //return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;
      return IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
    }

    bool terminateProgrammingState() 
    {
      TRC_INFORMATION("Terminating programming mode.");
      TRC_WARNING("Not implemented");
      //return true;
      return false;
    }

    void startListen()
    {
      m_runListenThread = true;
      m_listenThread = std::thread(&IqrfTcp::Imp::listen, this);
    }

    IIqrfChannelService::State getState() const
    {
      IIqrfChannelService::State state = State::NotReady;
      if (m_accessControl.hasExclusiveAccess())
        state = State::ExclusiveAccess;
      else if (m_runListenThread)
        state = State::Ready;

      return state;
    }

    std::unique_ptr<IIqrfChannelService::Accessor>  getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
    {
      return m_accessControl.getAccess(receiveFromFunc, access);
    }

    bool hasExclusiveAccess() const
    {
      return m_accessControl.hasExclusiveAccess();
    }

    IIqrfChannelService::osInfo getTrModuleInfo()
    {
      TRC_FUNCTION_ENTER("");
      TRC_WARNING("Reading TR module identification - not implemented.");

      IIqrfChannelService::osInfo myOsInfo;
      memset(&myOsInfo, 0, sizeof(myOsInfo));

      TRC_FUNCTION_LEAVE("");
      return myOsInfo;
    }

    /**
     * Reads information from component configuration used to establish a TCP connection.
     * Retrieves all possible hosts from read configuration.
     * Creates a socket on the file descriptor.
     * Attempts to establish a connection.
     *
     * @param props shape component configuration
     */
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfTcp instance activate" << std::endl <<
        "******************************"
      );

      using namespace rapidjson;

      try {
        uint16_t portnum = 0;
        std::string addrStr;
        struct addrinfo *dest, *res;
        struct addrinfo resolve;
        Document d;
        d.CopyFrom(props->getAsJson(), d.GetAllocator());

        //read target server address from configuration
        Value* address = Pointer("/address").Get(d);
        if (address != nullptr && address->IsString()) {
          addrStr = address->GetString();
        } else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /address");
        }

        //read target port from configuration
        Value* port = Pointer("/port").Get(d);
        if (port != nullptr && port->IsInt()) {
          //convert port number to network byte order
          portnum = htons(port->GetInt());
        } else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /port");
        }

        /************************************** Beginning of citation ************************************
         * The following section is inspired by a linux manual page for the getaddrinfo function.
         * The original example has been altered.
         * 
         * Title: getaddrinfo(3) - linux manual page
         * Author(s): Michael Kerrisk <mtk.manpages@gmail.com>, Ulrich Drepper <drepper@redhat.com>,
         *            Sam Varshavchik <mrsam@courier-mta.com>
         * Cited: 2020-06-02
         * License: https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.license.html
         * Availability: https://www.man7.org/linux/man-pages/man3/getaddrinfo.3.html
         */
        //specify connection parameters for host resolution
        memset(&resolve, 0, sizeof(struct addrinfo));
        resolve.ai_family = AF_UNSPEC;
        resolve.ai_socktype = SOCK_STREAM;
        resolve.ai_flags = 0;
        resolve.ai_protocol = 0;    

        //retrieve hosts from address and specified connection parameters
        if (getaddrinfo(&(*addrStr.c_str()), nullptr, &resolve, &res) != 0) {
          THROW_EXC_TRC_WAR(std::logic_error, "Failed to retrieve addr structures.");
        }
        struct sockaddr_in *addr;
        struct sockaddr_in6 *addr6;
        //iterate over retrieved results in attempt to establish a connection
        for(dest = res; dest != nullptr; dest = dest->ai_next) {
          //IPv4 connection
          if (dest->ai_family == AF_INET) {
            addr = (struct sockaddr_in*) dest->ai_addr;
            addr->sin_port = portnum;
            //create socket for ipv4 connection
            sockfd = socket(dest->ai_family, dest->ai_socktype, dest->ai_protocol);
            
            //failed to create socket, continue with the next result
            if (sockfd == -1) {
              continue;
            }

            //attempt to establish a connection, if successful, break the loop
            if (connect(sockfd, (struct sockaddr*) addr, sizeof(struct sockaddr_in)) != -1) {
              break;
            }
          //IPv6 connection
          } else if (dest->ai_family == AF_INET6) {
            addr6 = (struct sockaddr_in6*) dest->ai_addr;
            addr6->sin6_port = portnum;
            //create socket for ipv6 connection
            sockfd = socket(dest->ai_family, dest->ai_socktype, dest->ai_protocol);
           
            //failed to create socket, continue with the next result
            if (sockfd == -1) {
              continue;
            }

            //attempt to establish a connection, if successful, break the loop
            if (connect(sockfd, (struct sockaddr *) addr6, sizeof(struct sockaddr_in6)) != -1) {
              break;
            }
          }
        }

        //free retreived results as they are no longer needed
        freeaddrinfo(res);

        //connection could not be established with either result
        if (dest == nullptr) {
          THROW_EXC_TRC_WAR(std::logic_error, "Address property is not a valid ip address or hostname.");
        }
        /************************************** End of citation ************************************/

        TRC_FUNCTION_LEAVE("")
      } catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "activate exception");
      }
    }

    /**
     * Deactivates the TCP component after closing the connection on socket file descriptor. 
     */
    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      m_runListenThread = false;

      TRC_DEBUG("joining udp listening thread");
      if (m_listenThread.joinable())
        m_listenThread.join();
      TRC_DEBUG("listening thread joined");

      if (sockfd == -1) {
        THROW_EXC_TRC_WAR(std::logic_error, "Socket is not open.")
      }

      shutdown(sockfd, SHUT_RDWR);
      close(sockfd);

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "IqrfTcp instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
    }

    /**
     * The client socket listens for incomming messages from a TCP server.
     * Messages are received and handled by messageHandler.
     */
    void listen()
    {
      TRC_FUNCTION_ENTER("thread starts");

      try {
        while (m_runListenThread)
        {
          if (sockfd == -1) {
            THROW_EXC_TRC_WAR(std::logic_error, "Socket is not open.")
          }

          int recvlen;

          //receive a message from server
          recvlen = recv(sockfd, buffer, BUFFER_SIZE-1, 0);
          if (recvlen == -1) {
            TRC_WARNING("Cannot receive response.");
            //connection closed
            if (errno == ENOTCONN) {
              fprintf(stderr, "Error receiving message: %s\n", strerror(errno));
              THROW_EXC_TRC_WAR(std::logic_error, "Socket is not connected.");
            }
            continue;
          }

          buffer[BUFFER_SIZE-1] = '\0';
          m_rec = (unsigned char*)malloc(recvlen);

          if (m_rec == NULL) {
            TRC_WARNING("Cannot allocate memory for request.");
            continue;
          }

          //copy message content and clear buffer before another message is received
          memcpy(m_rec, buffer, recvlen);
          memset(buffer, 0, BUFFER_SIZE);

          if (recvlen > 0) {
            TRC_DEBUG(PAR(recvlen));
            std::basic_string<unsigned char> message(m_rec, recvlen);
            m_accessControl.messageHandler(message);
          }
        }
      }
      catch (std::logic_error& e) {
        CATCH_EXC_TRC_WAR(std::logic_error, e, "listening thread error");
        m_runListenThread = false;
      }
      TRC_WARNING("thread stopped");
    }

  private:
    AccessControl<IqrfTcp::Imp> m_accessControl;

    std::atomic_bool m_runListenThread;
    std::thread m_listenThread;

    unsigned char* m_rec = nullptr;
    unsigned m_bufsize = BUFFER_SIZE;
  };

  //////////////////////////////////////////////////
  IqrfTcp::IqrfTcp()
  {
    m_imp = shape_new Imp();
  }

  IqrfTcp::~IqrfTcp()
  {
    delete m_imp;
  }

  void IqrfTcp::startListen()
  {
    return m_imp->startListen();
  }

  IIqrfChannelService::State IqrfTcp::getState() const
  {
    return m_imp->getState();
  }

  std::unique_ptr<IIqrfChannelService::Accessor>  IqrfTcp::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
  {
    return m_imp->getAccess(receiveFromFunc, access);
  }

  bool IqrfTcp::hasExclusiveAccess() const
  {
    return m_imp->hasExclusiveAccess();
  }

  void IqrfTcp::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void IqrfTcp::deactivate()
  {
    m_imp->deactivate();
  }

  void IqrfTcp::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void IqrfTcp::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfTcp::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
