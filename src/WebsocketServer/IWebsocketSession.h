#pragma once

#include "WebsocketCallbackTypes.h"

namespace iqrf {

  class IWebsocketSession {
  public:
    virtual ~IWebsocketSession() {};

    virtual std::size_t getId() const = 0;

    virtual const std::string& getAddress() const = 0;

    virtual uint16_t getPort() const = 0;

    virtual void send(const std::string& message) = 0;

    virtual void run() = 0;

    virtual void close() = 0;

    virtual void setOnOpen(WsServerOnOpen) = 0;

    virtual void setOnClose(WsServerOnClose) = 0;

    virtual void setOnMessage(WsServerOnMessage) = 0;

    virtual void setOnAuth(WsServerOnAuth) = 0;

    virtual bool isAuthenticated() = 0;

  };
}
