/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
		 * @param messaging Messaging
		 * @param msg Message to send
		 */
		void sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg) override;

		/**
		 * Check if messaging accepts asynchronous messages
		 * @return true if messaging accepts asynchronous messages, false otherwise
		 */
		bool acceptAsyncMsg() const override { return false; }

		/**
		 * Return messaging instance
		 * @return Messagign instance
		 */
		const MessagingInstance &getMessagingInstance() const override;

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
		/// UDP channel
		UdpChannel* m_udpChannel = nullptr;
		/// UDP message queue
		TaskQueue<std::basic_string<uint8_t>>* m_toUdpMessageQueue = nullptr;
		/// Messaging instance
		MessagingInstance m_messagingInstance = MessagingInstance(MessagingType::UDP);
		/// Message handler
		IMessagingService::MessageHandlerFunc m_messageHandler;
	};
}
