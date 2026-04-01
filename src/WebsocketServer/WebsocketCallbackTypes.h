/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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

#pragma once

#include "boost/beast/core/error.hpp"

#include <chrono>
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
 * @param `std::chrono::system_clock::time_point&` token expiration to populate
 */
using WebSocketAuthHandler = std::function<boost::system::error_code(const std::size_t, const uint32_t, const std::string&, std::chrono::system_clock::time_point&, bool&)>;

/**
 * @brief Callback type for handling WebSocket connections closing.
 *
 * Used by the WebSocket server to notify application
 * code when a connection closes.
 *
 * @param `std::size_t` session ID
 */
using WebSocketCloseHandler = std::function<void(std::size_t)>;
