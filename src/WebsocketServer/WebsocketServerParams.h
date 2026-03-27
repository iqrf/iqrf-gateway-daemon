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

#include <cstdint>
#include <string>
#include <stdexcept>

#define DEFAULT_PORT 80
#define DEFAULT_TLS_PORT 443
#define DEFAULT_AUTH_TIMEOUT 30
#define DEFAULT_MAX_CLIENTS 50

namespace iqrf {

  /**
   * @enum class TransportModes
   * @brief Transport mode options for protocols / security
   */
  enum class TransportModes {
    PLAIN = 0,    ///< Plain connections
    TLS = 1,      ///< TLS connections
    BOTH = 2,     ///< Plain and TLS connections
  };

  /**
   * @brief Constructs TransportModes enum class member from string alue
   * @param value Transport mode value
   * @return `TransportModes` TransportModes representation
   *
   * @throws `std::invalid_argument` If TransportModes enum does not include value
   */
  inline TransportModes transportModeFromValue(const std::string& value) {
    if (value == "plain") {
      return TransportModes::PLAIN;
    }
    if (value == "tls") {
      return TransportModes::TLS;
    }
    if (value == "both") {
      return TransportModes::BOTH;
    }
    throw std::invalid_argument("Unknown or unsupported transport mode value.");
  }

  /**
   * @enum class TlsModes
   * @brief Mozilla-recommended TLS configuration levels
   */
  enum class TlsModes {
    MODERN = 0,         ///< Modern clients
    INTERMEDIATE = 1,   ///< General-purpose for most systems
    OLD = 2,            ///< Support for old clients
  };

  /**
   * @brief Constructs TlsModes enum class member from string value
   * @param value TLS mode value
   * @return TlsModes representation
   *
   * @throws `std::invalid_argument` If TlsModes enum does not include value
   */
  inline TlsModes tlsModeFromValue(const std::string& value) {
    if (value == "modern") {
      return TlsModes::MODERN;
    }
    if (value == "intermediate") {
      return TlsModes::INTERMEDIATE;
    }
    if (value == "old") {
      return TlsModes::OLD;
    }
    throw std::invalid_argument("Unknown or unsupported TLS mode value.");
  }

  /**
   * @struct WebsocketServerParams
   * @brief Configuration parameters for initialization of WebSocket Server
   */
  struct WebsocketServerParams {
    /**
     * @brief Server instance
     *
     * The purpose of instance is primarily for logging to differentiate between servers.
     */
    std::string instance;
    /**
     * @brief Port which the server listens on.
     */
    uint16_t port;
    /**
     * @brief Allow only localhost connections
     *
     * If `true`, the socket is bound to localhost.
     */
    bool localhostOnly;
    /**
     * @brief Transport mode
     *
     * Available modes (plain, tls, both)
     */
    TransportModes transportMode;
    /**
     * @brief TLS mode
     *
     * Available modes (modern, intermediate, old).
     */
    TlsModes tlsMode;
    /**
     * @brief TLS port which the server listens on.
     */
    uint16_t tlsPort;
    /**
     * @brief Path to certificate file
     *
     * Required to not be empty when TLS is enabled.
     */
    std::string certPath;
    /**
     * @brief Path to key file
     *
     * Required to not be empty when TLS is enabled.
     */
    std::string keyPath;
    /**
     * @brief Client session authentication timeout
     */
    uint16_t authTimeout;
    /**
     * @brief Maximum number of connected clients
     */
    std::size_t maxClients;

    /**
     * @brief Default constructor
     *
     * Initializes the websocket server params with: instance default, port 80, localhostOnly false, transportMode plain,
     * tlsMode intermediate, tls port 443, empty certificate and key path, authentication timeout of 30 seconds and maximum of 50 clients
     */
    WebsocketServerParams()
      : instance("default"), port(DEFAULT_PORT), localhostOnly(false), transportMode(TransportModes::PLAIN),
        tlsMode(TlsModes::INTERMEDIATE), tlsPort(DEFAULT_TLS_PORT), certPath(""), keyPath(""), authTimeout(DEFAULT_AUTH_TIMEOUT),
        maxClients(DEFAULT_MAX_CLIENTS) {}

    /**
     * @brief Parameterized constructor
     *
     * Initializes the websocket server params with specified parameters.
     *
     * @param instance Instance name
     * @param port Port number
     * @param localhostOnly Only accept connections from localhost
     * @param transportMode Transport mode
     * @param tlsMode TLS configuration
     * @param tlsPort TLS port number
     * @param certPath Path to certificate file
     * @param keyPath Path to private key file
     * @param authTimeout Authentication timeout in seconds
     * @param maxClients Maximum number of connected clients
     */
    WebsocketServerParams(const std::string& instance, uint16_t port, bool localhostOnly, TransportModes transportMode,
      TlsModes tlsMode, uint16_t tlsPort, const std::string& cert, const std::string& key, uint16_t authTimeout, std::size_t maxClients)
      : instance(instance), port(port), localhostOnly(localhostOnly), transportMode(transportMode),
        tlsMode(tlsMode), tlsPort(tlsPort), certPath(cert), keyPath(key), authTimeout(authTimeout), maxClients(maxClients) {}
  };
}


