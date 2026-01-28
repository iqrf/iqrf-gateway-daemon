#pragma once

#include "boost/beast/core/error.hpp"

#include <cstdint>
#include <functional>
#include <string>

/**
 * @brief Callback type for handling incoming WebSocket connections.
 *
 * Used by the WebSocket server to notify application
 * code when a client sends a message.
 *
 * @param `std::size_t` session ID
 */
using WebSocketOpenHandler = std::function<void(std::size_t)>;

/**
 * @brief Callback type for handling incoming WebSocket messages.
 *
 * Used by the WebSocket server to notify application
 * code when a client sends a message.
 *
 * @param `std::size_t` session ID
 * @param `std::string&` message payload
 */
using WebSocketMessageHandler = std::function<void(const std::size_t, const std::string&)>;

/**
 * @brief Callback type for handling incoming WebSocket auth requests.
 *
 * Used by the WebSocket server to notify application
 * code when a client sends an auth request.
 *
 * @param `std::size_t` session ID
 * @param `uint32_t` token ID
 * @param `std::string&` key
 * @param `int64_t&` token expiration to populate
 */
using WebSocketAuthHandler = std::function<boost::system::error_code(const std::size_t, const uint32_t, const std::string&, int64_t&, bool&)>;

/**
 * @brief Callback type for handling WebSocket connections closing.
 *
 * Used by the WebSocket server to notify application
 * code when a connection closes.
 *
 * @param `std::size_t` session ID
 */
using WebSocketCloseHandler = std::function<void(std::size_t)>;
