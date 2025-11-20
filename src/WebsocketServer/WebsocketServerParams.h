#pragma once

#include <functional>
#include <optional>
#include <string>
#include <stdexcept>

namespace iqrf {

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
   * @brief Constructs TlsModes enum class member from integer value
   * @param value TLS mode value
   * @return TlsModes representation
   *
   * @throws `std::invalid_argument` If TlsModes enum does not include value
   */
  inline TlsModes tlsModeFromValue(unsigned int value) {
    switch (value) {
      case 0:
        return TlsModes::MODERN;
      case 1:
        return TlsModes::INTERMEDIATE;
      case 2:
        return TlsModes::OLD;
      default:
        throw std::invalid_argument("Unknown or unsupported TLS mode value.");
    }
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
     * @brief Accept only TLS-secured connections
     */
    bool tls;
    /**
     * @brief TLS mode
     *
     * Available modes (modern, intermediate, old).
     */
    TlsModes tlsMode;
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
     * @brief Default constructor
     *
     * Initializes the websocket server params with: instance default, port 80, localhostOnly false, tls false,
     * tlsMode intermediate, empty certificate and key path
     */
    WebsocketServerParams()
      : instance("default"), port(80), localhostOnly(false), tls(false),
        tlsMode(TlsModes::INTERMEDIATE), certPath(""), keyPath("") {}

    /**
     * @brief Parameterized constructor
     *
     * Initializes the websocket server params with specified parameters.
     *
     * @param instance Instance name
     * @param port Port number
     * @param localhostOnly Only accept connections from localhost
     * @param tls Require TLS connections
     * @param tlsMode TLS configuration
     * @param certPath Path to certificate file
     * @param keyPath Path to private key file
     */
    WebsocketServerParams(const std::string& instance, uint16_t port, bool localhostOnly, bool tls,
      TlsModes tlsMode, const std::string& cert, const std::string& key)
      : instance(instance), port(port), localhostOnly(localhostOnly), tls(tls),
        tlsMode(tlsMode), certPath(cert), keyPath(key) {}
  };
}


