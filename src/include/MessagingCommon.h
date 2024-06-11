#pragma once

#include <stdexcept>
#include <string>
#include <tuple>

namespace iqrf {

  /// @brief Messaging type enum
  enum class MessagingType {
    MQ,
    MQTT,
    BUFFERED_MQTT,
    WS,
    UDP,
    SCHEDULER,
    TEST,
  };

  /// Utility functions for conversion of messaging type from and to string
  class MessagingConversion {
  public:
    /**
     * Converts messaging type enum value into string representation
     * @param type Messaging type enum value
     * @return std::string String representation of messaging type
     */
    static std::string messagingTypeToString(const MessagingType &type) {
      switch (type) {
        case MessagingType::MQ:
          return "mq";
        case MessagingType::MQTT:
          return "mqtt";
        case MessagingType::BUFFERED_MQTT:
          return "bmqtt";
        case MessagingType::WS:
          return "ws";
        case MessagingType::UDP:
          return "udp";
        case MessagingType::SCHEDULER:
          return "scheduler";
        case MessagingType::TEST:
          return "test";
        default:
          throw std::domain_error("Unknown messaging type value.");
      }
    }

    /**
     * Converts messaging type string into enum value
     * @param type Messaging type string
     * @return MessagingType Messaging type enum value
     */
    static MessagingType messagingTypeFromString(const std::string &type) {
      if (type == "mq") {
        return MessagingType::MQ;
      }
      if (type == "mqtt") {
        return MessagingType::MQTT;
      }
      if (type == "bmqtt") {
        return MessagingType::BUFFERED_MQTT;
      }
      if (type == "ws") {
        return MessagingType::WS;
      }
      if (type == "udp") {
        return MessagingType::UDP;
      }
      if (type == "scheduler") {
        return MessagingType::SCHEDULER;
      }
      if (type == "test") {
        return MessagingType::TEST;
      }
      throw std::domain_error("Unknown messaging type string.");
    }
  };


  /// @brief MessagingInstance represents an instance of messaging service by type and instance name
  class MessagingInstance {
  public:
    MessagingInstance() = delete;

    /**
     * Messaging type string constructor
     * @param type Messaging type as string
     * @param instance Instance name
     */
    MessagingInstance(const std::string &type, const std::string &instance = "default") {
      this->type = MessagingConversion::messagingTypeFromString(type);
      this->instance = instance;
    }

    /**
     * Messaging type enum member constructor
     * @param type Messaging type as enum member
     * @param instance Instance name
     */
    MessagingInstance(const MessagingType &type, const std::string &instance = "default"): type(type), instance(instance) {};

    friend bool operator==(const MessagingInstance& a, const MessagingInstance& b) {
      return a.type == b.type && a.instance == b.instance;
    }

    friend bool operator<(const MessagingInstance& a, const MessagingInstance& b) {
      return std::tie(a.type, a.instance) < std::tie(b.type, b.instance);
    }

    friend bool operator>(const MessagingInstance& a, const MessagingInstance& b) { return b < a; }
    friend bool operator<=(const MessagingInstance& a, const MessagingInstance& b) { return !(b < a); }
    friend bool operator>=(const MessagingInstance& a, const MessagingInstance& b) { return !(a < b); }

    /**
     * Serialize messaging instance object to string in format <type>-<instance_name>
     * @return const std::string String serialized messaging instance
     */
    const std::string to_string() const {
      return MessagingConversion::messagingTypeToString(this->type) + '-' + this->instance;
    }

    /// Messaging type
    MessagingType type;
    /// Instance name
    std::string instance;
  };
}
