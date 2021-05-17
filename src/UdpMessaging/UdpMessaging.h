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
		 * @param messagingId Messaging ID
		 * @param msg Message to send
		 */
		void sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg) override;
		
		/**
		 * Returns instance name
		 * @return Instance name
		 */
		const std::string& getName() const override;

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

		/// Instance name
		std::string m_name;
		/// Port to send to
		int m_remotePort = 55000;
		/// Port to listen on
		int m_localPort = 55300;
		/// UDP channel
		UdpChannel* m_udpChannel = nullptr;
		/// UDP message queue
		TaskQueue<std::basic_string<uint8_t>>* m_toUdpMessageQueue = nullptr;
		/// Message handler
		IMessagingService::MessageHandlerFunc m_messageHandler;
	};
}
