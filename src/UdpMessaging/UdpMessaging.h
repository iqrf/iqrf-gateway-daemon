/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include "IUdpMessagingService.h"
#include "TaskQueue.h"
#include "UdpChannel.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrf {
	/// UDP messaging class
	class UdpMessaging : public IUdpMessagingService {
	public:
		/**
		 * UDP messaging constructor
		 */
		UdpMessaging();

		/**
		 * UDP messaging destructor
		 */
		virtual ~UdpMessaging();

		/**
		 * Sets message handler for received messages
		 * @param messageHandler Function to pass received message to
		 */
		void registerMessageHandler(MessageHandlerFunc messageHandler) override;

		/**
		 * Clears handler for received messages
		 */
		void unregisterMessageHandler() override;

		/**
		 * Sends response via UDP channel
		 * @param messaging Messaging instance
		 * @param msg Message to send
		 */
		void sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg) override;

		/**
		 * Returns IP address of receiving interface
		 * @return IP address of receiving interface
		 */
		const std::string& getListeningIpAddress() const override;

		/**
		 * Returns MAC address of receiving interface
		 * @return MAC address of receiving interface
		 */
		const std::string& getListeningMacAddress() const override;

		/**
		 * Returns listening port
		 * @return Listening port
		 */
		unsigned short getListeningIpPort() const override;

		/**
		 * Checks if messaging accepts asynchronos messages
		 * @return false, dummpy impl
		 */
		bool acceptAsyncMsg() const override { return false; }

		/**
		 * Return messaging instance
		 * @return Messaging instance
		 */
		const MessagingInstance& getMessagingInstance() const override { return m_messagingInstance; }

		/**
		 * Initializes component instance
		 * @param props Component instance properties
		 */
		void activate(const shape::Properties *props = 0);

		/**
		 * Modifies component instance properties
		 * @param props Component instance properties
		 */
		void modify(const shape::Properties *props);

		/**
		 * Deactivates messaging component instance
		 */
		void deactivate();

		/**
		 * Attaches a tracing service observer
		 * @param iface Tracing service observer
		 */
		void attachInterface(shape::ITraceService* iface);

		/**
		 * Detaches a tracing service observer
		 * @param iface Tracing service observer
		 */
		void detachInterface(shape::ITraceService* iface);

	private:
		/**
		 * Passes received message to message handler
		 * @param message Received message
		 */
		int handleMessageFromUdp(const std::basic_string<uint8_t> & message);

		/// Port to send to
		int m_remotePort = 55000;
		/// Port to listen on
		int m_localPort = 55300;
		/// Network device expiration
		int m_expiration;
		/// Messaging instance
		MessagingInstance m_messagingInstance = MessagingInstance(MessagingType::UDP);
		/// UDP channel
		UdpChannel* m_udpChannel = nullptr;
		/// UDP message queue
		TaskQueue<std::basic_string<uint8_t>>* m_toUdpMessageQueue = nullptr;
		/// Message handler
		IMessagingService::MessageHandlerFunc m_messageHandler;
	};
}
