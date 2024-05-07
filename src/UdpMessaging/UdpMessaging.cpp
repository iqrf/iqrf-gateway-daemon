/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#define IUdpMessagingService_EXPORTS
#define IMessagingService_EXPORTS

#include "UdpMessaging.h"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "Trace.h"

#include "iqrf__UdpMessaging.hxx"

TRC_INIT_MODULE(iqrf::UdpMessaging)

const unsigned IQRF_MQ_BUFFER_SIZE = 64 * 1024;

namespace iqrf {
	UdpMessaging::UdpMessaging() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	UdpMessaging::~UdpMessaging() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	void UdpMessaging::sendMessage(const MessagingInstance& messaging, const std::basic_string<uint8_t> & msg) {
		TRC_FUNCTION_ENTER(PAR(messaging.instance));

		TRC_DEBUG(MEM_HEX_CHAR(msg.data(), msg.size()));
		m_toUdpMessageQueue->pushToQueue(msg);

		TRC_FUNCTION_LEAVE("")
	}

	int UdpMessaging::handleMessageFromUdp(const std::basic_string<uint8_t> & message) {
		TRC_DEBUG("==================================" << std::endl <<
		"Received from UDP: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

		if (m_messageHandler) {
			m_messageHandler(m_messagingInstance, std::vector<uint8_t>(message.data(), message.data() + message.size()));
		}

		return 0;
	}

	void UdpMessaging::registerMessageHandler(MessageHandlerFunc messageHandler) {
		TRC_FUNCTION_ENTER("");
		m_messageHandler = messageHandler;
		TRC_FUNCTION_LEAVE("");
	}

	void UdpMessaging::unregisterMessageHandler() {
		TRC_FUNCTION_ENTER("");
		m_messageHandler = IMessagingService::MessageHandlerFunc();
		TRC_FUNCTION_LEAVE("");
	}

	const MessagingInstance& UdpMessaging::getMessagingInstance() const {
		return m_messagingInstance;
	}

	const std::string& UdpMessaging::getListeningIpAddress() const {
		return m_udpChannel->getListeningIpAddress();
	}

	unsigned short UdpMessaging::getListeningIpPort() const {
		return m_udpChannel->getListeningIpPort();
	}

	const std::string& UdpMessaging::getListeningMacAddress() const {
		return m_udpChannel->getListeningMacAddress();
	}

	void UdpMessaging::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");

		TRC_INFORMATION(std::endl <<
		"******************************" << std::endl <<
		"UdpMessaging instance activate" << std::endl <<
		"******************************"
		);

		std::string instanceName;

		props->getMemberAsString("instance", instanceName);
		props->getMemberAsInt("RemotePort", m_remotePort);
		props->getMemberAsInt("LocalPort", m_localPort);
		props->getMemberAsInt("deviceRecordExpiration", m_expiration);

		m_messagingInstance.instance = instanceName;

		m_udpChannel = shape_new UdpChannel(m_remotePort, m_localPort, m_expiration, IQRF_MQ_BUFFER_SIZE);

		m_toUdpMessageQueue = shape_new TaskQueue<std::basic_string<uint8_t>>([&](const std::basic_string<uint8_t>& msg) {
			m_udpChannel->sendTo(msg);
		});

		m_udpChannel->registerReceiveFromHandler([&](const std::basic_string<unsigned char>& msg) -> int {
			return handleMessageFromUdp(msg);
		});

		TRC_FUNCTION_LEAVE("")
	}

	void UdpMessaging::modify(const shape::Properties *props) {
		(void)props;
	}

	void UdpMessaging::deactivate() {
		TRC_FUNCTION_ENTER("");

		m_udpChannel->unregisterReceiveFromHandler();

		delete m_udpChannel;
		delete m_toUdpMessageQueue;

		TRC_INFORMATION(std::endl <<
		"******************************" << std::endl <<
		"UdpMessaging instance deactivate" << std::endl <<
		"******************************"
		);
		TRC_FUNCTION_LEAVE("")
	}

	void UdpMessaging::attachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void UdpMessaging::detachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().removeTracerService(iface);
	}

}
