#pragma once

#include "boost/beast/core/error.hpp"
#include "boost/beast/websocket/stream.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace iqrf {

  using WsStreamTls = boost::beast::websocket::stream<boost::asio::ssl::stream<boost::beast::tcp_stream>>;
  using WsStreamPlain = boost::beast::websocket::stream<boost::beast::tcp_stream>;

  class IWebsocketSession {
  public:
    virtual ~IWebsocketSession() {};

    virtual std::size_t getId() const = 0;

    virtual const std::string& getAddress() const = 0;

    virtual uint16_t getPort() const = 0;

    virtual void send(const std::string& message) = 0;

    virtual void run() = 0;

    virtual void close() = 0;

    virtual void setOnOpen(std::function<void(std::size_t)>) = 0;

    virtual void setOnClose(std::function<void(std::size_t)>) = 0;

    virtual void setOnMessage(std::function<void(const std::size_t, const std::string&)>) = 0;

  };
}
