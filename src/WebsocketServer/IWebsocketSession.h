#pragma once

#include "boost/beast/core/error.hpp"

#include <cstdint>
#include <functional>
#include <string>

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

    virtual void setOnOpen(std::function<void(std::size_t, boost::beast::error_code)>) = 0;

    virtual void setOnClose(std::function<void(std::size_t, boost::beast::error_code)>) = 0;

    virtual void setOnMessage(std::function<void(const std::size_t, const std::string&)>) = 0;

  };
}
