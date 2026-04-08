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

#include <atomic>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include "boost/beast/core/error.hpp"
#include "boost/beast/websocket/stream.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
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

  /// @brief Session call type
  using SessionHandler = std::function<void(std::shared_ptr<IWebSocketClientSession>)>;

  /**
   * @brief Session manager class
   *
   * This class holds a map of sessions, intended for use with websocket server class.
   * When a new session is created, the session registry reference is passed into the session,
   * so that if the session can be accepted, it can register itself, and unregister on termination.
   *
   * The session manager also maintains session ID counter, that allows callers to fetch next session ID
   * and the internal counter is incremented. The used counter is atomic, to prevent race conditions.
   *
   * Session is created with a capacity, specifying number of concurrent sessions (connected clients).
   */
  class SessionManager {
  private:
    /// Atomic session ID counter
    std::atomic_size_t sessionIdCounter_{0};
    /// Maximum client session count
    std::size_t capacity_{50};
    /// Map of stored sessions
    std::unordered_map<std::size_t, std::weak_ptr<IWebSocketClientSession>> sessionStorage_;
    /// Session map access mutex
    mutable std::mutex mutex_;
  public:
    /**
     * Constructs a session manager with capacity
     * @param capacity Session manager capacity
     */
    explicit SessionManager(std::size_t capacity) : capacity_(capacity) {}

    /**
     * Returns next session ID and updates the counter
     * @return `std::size_t` Next session ID
     */
    std::size_t getNextSessionId() {
      return sessionIdCounter_.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * Registers session with ID
     *
     * The method checks if session storage is full (against the capacity),
     * and if there is no more space for the new client, returns false.
     *
     * If the session can be accepted, it is stored and the method returns true.
     *
     * @param id Session ID
     * @param session Session to store
     * @return `true` if session can be accepted, `false` otherwise
     */
    bool registerSession(std::size_t id, std::shared_ptr<IWebSocketClientSession> session) {
      std::lock_guard<std::mutex> lock(mutex_);

      if (sessionStorage_.size() >= capacity_) {
        return false;
      }

      // TODO: if ID exists, do not add, do not override session map

      sessionStorage_[id] = session;
      return true;
    }

    /**
     * Finds a session by ID and returns pointer to it
     *
     * The method returns a temporary shared pointer to session that should be destroyed
     * once the session pointer is no longer needed, no further actions need to be performed
     * on the session.
     *
     * @param id Session ID
     * @return Shared pointer to session object or null pointer if session does not exist
     */
    std::shared_ptr<IWebSocketClientSession> getSession(std::size_t id) {
      std::lock_guard<std::mutex> lock(mutex_);
      auto record = sessionStorage_.find(id);
      if (record == sessionStorage_.end()) {
        return nullptr;
      }
      auto session = record->second.lock();
      if (!session) {
        return nullptr;
      }
      return session;
    }

    /**
     * Pass a function to call on every available session
     *
     * This method is useful for actions to be performed on all sessions
     * such as broadcasting a message, or closing all sessions at once.
     *
     * @param handler Function to call
     */
    template<class SessionHandler>
    void forEachSession(SessionHandler&& handler) {
      std::vector<std::shared_ptr<IWebSocketClientSession>> living;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto itr = sessionStorage_.begin(); itr != sessionStorage_.end(); ) {
          if (auto sessionPointer = itr->second.lock()) {
            living.push_back(sessionPointer);
            ++itr;
          } else {
            itr = sessionStorage_.erase(itr);
          }
        }
      }

      for (auto& session : living) {
        handler(session);
      }
    }

    /**
     * Unregister session by ID
     * @param id Session ID
     * @return `true` if session existed and was removed from storage, `false` otherwise
     */
    bool unregisterSession(std::size_t id) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (sessionStorage_.count(id)) {
        sessionStorage_.erase(id);
        return true;
      }
      return false;
    }
  };
}
