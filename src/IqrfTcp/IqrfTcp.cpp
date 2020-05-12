#define IIqrfChannelService_EXPORTS

#include "IqrfTcp.h"
#include "AccessControl.h"
#include "rapidjson/pointer.h"
#include <mutex>
#include <regex>
#include <thread>
#include <atomic>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

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
int sockfd;
//int clientfd;
char buffer[BUFFER_SIZE];

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

    void send(const std::basic_string<unsigned char>& message)
    {
      TRC_INFORMATION("Sending to IQRF TCP: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));
      
      if ((::send(sockfd, message.data(), message.size(), 0)) == -1) {
        TRC_WARNING("Cannot send message.");
      } else {
        TRC_INFORMATION("Message successfully sent.");
      }

    }

    bool enterProgrammingState() {
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

    bool terminateProgrammingState() {
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
        int option = 1, portnum = 0;
        uint16_t convertedPort;
        std::string addrStr;
        struct addrinfo *dest, *res;
        struct addrinfo resolve;
        Document d;
        d.CopyFrom(props->getAsJson(), d.GetAllocator());

        Value* address = Pointer("/address").Get(d);
        if (address != nullptr && address->IsString()) {
          addrStr = address->GetString();
        } else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /address");
        }

        Value* port = Pointer("/port").Get(d);
        if (port != nullptr && port->IsInt()) {
          portnum = port->GetInt();
        } else {
          THROW_EXC_TRC_WAR(std::logic_error, "Cannot find property: /port");
        }

        memset(&resolve, 0, sizeof(struct addrinfo));
        resolve.ai_family = AF_UNSPEC;
        resolve.ai_socktype = SOCK_STREAM;
        resolve.ai_flags = 0;
        resolve.ai_protocol = 0;    
      
        getaddrinfo(&(*addrStr.c_str()), nullptr, &resolve, &res);
        convertedPort = htons(portnum);
        struct sockaddr_in *addr;
        struct sockaddr_in6 *addr6;
        for(dest = res; dest != nullptr; dest = dest->ai_next) {
          if (dest->ai_family == AF_INET) {
            addr = (struct sockaddr_in*) dest->ai_addr;
            addr->sin_port = convertedPort;
            sockfd = socket(dest->ai_family, dest->ai_socktype, dest->ai_protocol);
            
            if (sockfd == -1) {
              continue;
            }

            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
              THROW_EXC_TRC_WAR(std::logic_error, "Address or port is already in use.");
            }

            if (connect(sockfd, (struct sockaddr*) addr, sizeof(struct sockaddr_in)) != -1) {
              break;
            }
          } else if (dest->ai_family == AF_INET6) {
            addr6 = (struct sockaddr_in6*) dest->ai_addr;
            addr6->sin6_port = convertedPort;
            sockfd = socket(dest->ai_family, dest->ai_socktype, dest->ai_protocol);
           
            if (sockfd == -1) {
              continue;
            }

            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option))) {
              THROW_EXC_TRC_WAR(std::logic_error, "Address or port is already in use.");
            }

            if (connect(sockfd, (struct sockaddr *) addr6, sizeof(struct sockaddr_in6)) != -1) {
              break;
            }
          }
        }

        freeaddrinfo(res);

        if (dest == nullptr) {
          THROW_EXC_TRC_WAR(std::logic_error, "Address property is not a valid ip address or hostname.");
        }

        TRC_FUNCTION_LEAVE("")
      } catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "activate exception");
      }
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      m_runListenThread = false;

      TRC_DEBUG("joining udp listening thread");
      if (m_listenThread.joinable())
        m_listenThread.join();
      TRC_DEBUG("listening thread joined");

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

    void listen()
    {
      TRC_FUNCTION_ENTER("thread starts");

      try {
        while (m_runListenThread)
        {
          int recvlen;

          recvlen = recv(sockfd, buffer, BUFFER_SIZE-1, 0);
          if (recvlen == -1) {
            perror(NULL);
            TRC_WARNING("Cannot receive response.");
            continue;
          }

          buffer[BUFFER_SIZE-1] = '\0';
          m_rec = (unsigned char*)malloc(recvlen);

          if (m_rec == NULL) {
            TRC_WARNING("Cannot allocate memory for request.");
            continue;
          }

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
