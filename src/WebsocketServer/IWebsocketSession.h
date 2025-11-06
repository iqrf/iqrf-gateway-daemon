#pragma once

#include "boost/beast/core/error.hpp"
#include "boost/beast/websocket/stream.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include "WebsocketCallbackTypes.h"

namespace iqrf {

  /// @brief TLS stream type
  using WsStreamTls = boost::beast::websocket::stream<boost::asio::ssl::stream<boost::beast::tcp_stream>>;
  /// @brief Plain stream type
  using WsStreamPlain = boost::beast::websocket::stream<boost::beast::tcp_stream>;

  /**
   * @interface IWebsocketSession
   * @brief Interface for WebSocket sessions
   */
  class IWebsocketSession {
  public:
    virtual ~IWebsocketSession() {};

    /**
     * @brief Get session ID
     * @return `std::size_t` Session ID
     */
    virtual std::size_t getId() const = 0;

    /**
     * @brief Get server host
     * @return `std::string&` Server host
     */
    virtual const std::string& getAddress() const = 0;

    /**
     * @brief Get server port
     * @return `uint16_t` Server port
     */
    virtual uint16_t getPort() const = 0;

    /**
     * @brief Send message to client(s)
     *
     * @param message Message to send
     */
    virtual void send(const std::string& message) = 0;

    /**
     * @brief Starts the session
     */
    virtual void run() = 0;

    /**
     * @brief Closes the session
     */
    virtual void close() = 0;

    /**
     * @brief Sets onOpen callback
     */
    virtual void setOnOpen(WsServerOnOpen) = 0;

    /**
     * @brief Sets onClose callback
     */
    virtual void setOnClose(WsServerOnClose) = 0;

    /**
     * @brief Sets onMessage callback
     */
    virtual void setOnMessage(WsServerOnMessage) = 0;

    /**
     * @brief Sets onAuth callback
     */
    virtual void setOnAuth(WsServerOnAuth) = 0;

    /**
     * Checks if session is authenticated
     * @return `bool` true if session is authenticated, false otherwise
     */
    virtual bool isAuthenticated() = 0;

  };
}
