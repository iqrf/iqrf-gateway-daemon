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

#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

namespace iqrf {

	/// @brief Messaging type enum
	enum class MessagingType {
		MQTT,
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
				case MessagingType::MQTT:
					return "mqtt";
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
			if (type == "mqtt") {
				return MessagingType::MQTT;
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

    /**
     * Check if messaging instance has client session information
     */
    template<typename T>
    typename std::enable_if<std::is_same<T, std::size_t>::value || std::is_same<T, std::string>::value, bool>::type
    hasClientSession() const {
      return std::holds_alternative<T>(this->clientSession);
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, std::size_t>::value || std::is_same<T, std::string>::value, T>::type
    getClientSession() const {
      return std::get<T>(this->clientSession);
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, std::size_t>::value || std::is_same<T, std::string>::value, void>::type
    setClientSession(const T& val) {
      this->clientSession = val;
    }

		/// Messaging type
		MessagingType type;
		/// Instance name
		std::string instance;
    /// Session ID or topic
    std::variant<std::monostate, std::size_t, std::string> clientSession;
	};
}
