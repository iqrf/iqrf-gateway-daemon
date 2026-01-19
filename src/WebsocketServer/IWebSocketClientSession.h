#pragma once

#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include "boost/beast/core/error.hpp"
#include "boost/beast/websocket/stream.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include "WebsocketCallbackTypes.h"

namespace iqrf {

  /// @brief Plain stream type
  using PlainWebSocketStream = boost::beast::websocket::stream<boost::beast::tcp_stream>;
  /// @brief TLS stream type
  using TlsWebSocketStream = boost::beast::websocket::stream<boost::asio::ssl::stream<boost::beast::tcp_stream>>;

  /**
   * @interface IWebSocketClientSession
   * @brief Interface for WebSocket client sessions
   */
  class IWebSocketClientSession {
  public:
    virtual ~IWebSocketClientSession() {};

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
    virtual void sendMessage(std::string message) = 0;

    /**
     * @brief Starts the session
     */
    virtual void startSession() = 0;

    /**
     * @brief Closes the session
     *
     * @param cc Close code
     */
    virtual void closeSession(boost::beast::websocket::close_code cc) = 0;

    /**
     * @brief Register on connection open callback
     * @param onOpen On connection open callback
     */
    virtual void setOnOpen(WebSocketOpenHandler onOpen) = 0;

    /**
     * @brief Register on message received callback
     * @param onMessage On message received callback
     */
    virtual void setOnMessage(WebSocketMessageHandler onMessage) = 0;

    /**
     * @brief Registers authentication callback
     * @param onAuth Authentication callback
     */
    virtual void setAuthCallback(WebSocketAuthHandler onAuth) = 0;

    /**
     * @brief Registers on connection close callback
     * @param onClose On connection close callback
     */
    virtual void setOnClose(WebSocketCloseHandler onClose) = 0;

    /**
     * Checks if session is authenticated
     * @return `true` if session is authenticated, `false` otherwise
     */
    virtual bool isAuthenticated() = 0;

  };
}
