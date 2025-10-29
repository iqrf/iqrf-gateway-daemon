#include <functional>
#include <optional>
#include <string>
#include <stdexcept>

namespace iqrf {

  typedef std::function<void(const std::size_t, const std::string&)> WsServerOnMessage;

  enum class TlsModes {
    MODERN = 0,
    INTERMEDIATE = 1,
    OLD = 2,
  };

  static TlsModes tlsModeFromValue(unsigned int value) {
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

  struct WebsocketServerParams {
    /// Instance name
    std::string instance;
    /// Server port
    uint16_t port;
    /// Accept only localhost connections
    bool localhostOnly;
    /// TLS enabled
    bool tls;
    /// TLS mode (modern, intermediate, old)
    TlsModes tlsMode;
    /// Path to certificate file
    std::string certPath;
    /// Path to private key file
    std::string keyPath;

    WebsocketServerParams()
      : instance("default"), port(80), localhostOnly(false), tls(false),
        tlsMode(TlsModes::INTERMEDIATE), certPath(""), keyPath("") {}

    WebsocketServerParams(const std::string& instance, uint16_t port, bool localhostOnly, bool tls,
      TlsModes tlsMode, const std::string& cert = "", const std::string& key = "")
      : instance(instance), port(port), localhostOnly(localhostOnly), tls(tls), tlsMode(tlsMode), certPath(cert), keyPath(key) {}
  };
}


