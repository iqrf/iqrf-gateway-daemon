/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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

#define IUdpConnectorService_EXPORTS

#include "IdeCounterpart.h"

#include "iqrf__IdeCounterpart.hxx"

TRC_INIT_MODULE(iqrf::IdeCounterpart);

using namespace rapidjson;

namespace iqrf {

	IdeCounterpart::IdeCounterpart() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("")
	}

	IdeCounterpart::~IdeCounterpart() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("")
	}

	///// API implementation /////

	IUdpConnectorService::Mode IdeCounterpart::getMode() const {
		std::lock_guard<std::mutex> lck(m_modeMtx);
		return m_mode;
	}

	void IdeCounterpart::setMode(Mode mode) {
		TRC_FUNCTION_ENTER(NAME_PAR(mode, (int)mode));

		std::lock_guard<std::mutex> lck(m_modeMtx);

		if (mode == Mode::Operational) {
			m_exclusiveAcessor.reset();
			m_snifferAcessor.reset();
		} else if (mode == Mode::Forwarding) {
			m_exclusiveAcessor.reset();
			m_snifferAcessor = m_iqrfChannelService->getAccess([&](const std::basic_string<unsigned char>& received)->int {
				return sendMessageToIde(received);
			}, IIqrfChannelService::AccesType::Sniffer);
		} else if (mode == Mode::Service) {
			m_snifferAcessor.reset();
			m_exclusiveAcessor = m_iqrfChannelService->getAccess([&](const std::basic_string<unsigned char>& received)->int {
				return sendMessageToIde(received);
			}, IIqrfChannelService::AccesType::Exclusive);
		} else {
			return;
		}

		m_mode = mode;

		TRC_INFORMATION("Set mode " << ModeStringConvertor::enum2str(m_mode));
		TRC_FUNCTION_LEAVE("");
	}

	///// Message handling /////

	int IdeCounterpart::handleMsg(const std::vector<uint8_t> &msg) {
		TRC_DEBUG(std::endl
			<< "==================================" << std::endl
			<< "Received from UDP: " << std::endl
			<< MEM_HEX_CHAR(msg.data(), msg.size())
		);
		const std::basic_string<uint8_t> message(msg.data(), msg.size());

		try {
			validateMsg(message);
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::logic_error, e, e.what());
			return -1;
		}

		unsigned short dataLen = (message[DLEN_H] << 8) + message[DLEN_L];
		std::basic_string<unsigned char> data = message.substr(BaseCommand::HEADER_SIZE, dataLen);
		const uint8_t command = message[PacketHeader::CMD];
		std::unique_ptr<BaseCommand> handler;


		switch (command) {
			case UdpCommands::GW_IDENTIFICATION: {
				auto params = m_params;
				params.IP = m_messaging->getListeningIpAddress();
				params.MAC = m_messaging->getListeningMacAddress();
				handler = std::make_unique<GatewayIdentification>(GatewayIdentification(message, params, m_iqrfDpaService));
				break;
			}
			case UdpCommands::GW_STATUS:
				handler = std::make_unique<GatewayStatus>(GatewayStatus(message, m_exclusiveAcessor || m_snifferAcessor));
				break;
			case UdpCommands::TR_WRITE:
				handler = std::make_unique<TrWrite>(TrWrite(message, (m_exclusiveAcessor != nullptr)));
				break;
			case UdpCommands::TR_INFO:
				handler = std::make_unique<TrInfo>(TrInfo(message, m_iqrfDpaService));
				break;
			case UdpCommands::TR_RESET:
				handler = std::make_unique<TrReset>(TrReset(message, (m_exclusiveAcessor != nullptr)));
				data = TrReset::getDpaRequest();
				break;
			default:
				handler = std::make_unique<UnknownCommand>(UnknownCommand(message));
				TRC_DEBUG("Unknown or unsupported UDP command: " << PAR(command));
		}

		try {
			handler->buildResponse();
			m_messaging->sendMessage("", handler->getResponse());
			if (handler->isTrWriteRequired() && m_exclusiveAcessor) {
				m_exclusiveAcessor->send(data);
			}
			return 0;
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::logic_error, e, e.what());
			return -1;
		}
	}

	void IdeCounterpart::validateMsg(const std::basic_string<unsigned char> &message) {
		const size_t messageLen = message.size();

		if (messageLen < (BaseCommand::HEADER_SIZE + BaseCommand::CRC_SIZE)) {
			THROW_EXC_TRC_WAR(std::logic_error, "Message too short: " << PAR(messageLen));
		}

		if (messageLen > (BaseCommand::HEADER_SIZE + BaseCommand::DATA_MAX_SIZE + BaseCommand::CRC_SIZE)) {
			THROW_EXC_TRC_WAR(std::logic_error, "Message too long: " << PAR(messageLen));
		}

		if (message[GW_ADDR] != m_params.mode) {
			THROW_EXC_TRC_WAR(std::logic_error, "GW_ADDR mismatch: " << PAR_HEX(message[GW_ADDR]));
		}

		size_t dataLen = (message[DLEN_H] << 8) + message[DLEN_L];

		if (messageLen != (BaseCommand::HEADER_SIZE + dataLen + BaseCommand::CRC_SIZE)) {
			THROW_EXC_TRC_WAR(std::logic_error, "Message length does not match specified data length.");
		}

		unsigned short crc = (message[BaseCommand::HEADER_SIZE + dataLen] << 8) + message[BaseCommand::HEADER_SIZE + dataLen + 1];
		if (crc != Crc::get().GetCRC_CCITT(const_cast<unsigned char*>(reinterpret_cast<const unsigned char *>(message.data())), BaseCommand::HEADER_SIZE + dataLen)) {
			THROW_EXC_TRC_WAR(std::logic_error, "Invalid message CRC.");
		}
	}

	int IdeCounterpart::sendMessageToIde(const std::basic_string<unsigned char>& message) {
		SendTrData dataToSend(message);
		dataToSend.buildResponse();
		m_messaging->sendMessage("", dataToSend.getResponse());
		return 0;
	}

	///// Component management /////

	void IdeCounterpart::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "IdeCounterpart instance activate" << std::endl
			<< "******************************"
		);
		modify(props);
		m_messaging->registerMessageHandler([&](const std::string& messagingId, const std::vector<uint8_t>& msg) {
			(void)messagingId;  //silence -Wunused-parameter
			return handleMsg(msg);
		});
		TRC_FUNCTION_LEAVE("")
	}

	void IdeCounterpart::modify(const shape::Properties *props) {
		const Document& doc = props->getAsJson();

		const Value* val = Pointer("/gwIdentModeByte").Get(doc);
		if (val && val->IsUint()) {
			m_params.mode = static_cast<uint8_t>(val->GetUint());
		}

		val = Pointer("/gwIdentName").Get(doc);
		if (val && val->IsString()) {
			m_params.name = val->GetString();
		}

		val = Pointer("/gwIdentIpStack").Get(doc);
		if (val && val->IsString()) {
			m_params.ipStack = val->GetString();
		}

		val = Pointer("/gwIdentNetBios").Get(doc);
		if (val && val->IsString()) {
			m_params.netBios = val->GetString();
		}

		val = Pointer("/gwIdentPublicIp").Get(doc);
		if (val && val->IsString()) {
			m_params.publicIp = val->GetString();
		}

		IUdpConnectorService::Mode startupMode = IdeCounterpart::Mode::Operational;
		val = Pointer("/operMode").Get(doc);
		if (val && val->IsString()) {
			startupMode = ModeStringConvertor::str2enum(val->GetString());
		}

		setMode(startupMode);
	}

	void IdeCounterpart::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"IdeCounterpart instance deactivate" << std::endl <<
			"******************************"
		);
		setMode(IdeCounterpart::Mode::Operational);
		m_messaging->unregisterMessageHandler();

		TRC_FUNCTION_LEAVE("")
	}

	///// Interface management /////

	void IdeCounterpart::attachInterface(iqrf::IIqrfChannelService* iface) {
		m_iqrfChannelService = iface;
	}

	void IdeCounterpart::detachInterface(iqrf::IIqrfChannelService* iface) {
		if (m_iqrfChannelService == iface) {
			m_iqrfChannelService = nullptr;
		}
	}

	void IdeCounterpart::attachInterface(iqrf::IIqrfDpaService* iface) {
		m_iqrfDpaService = iface;
	}

	void IdeCounterpart::detachInterface(iqrf::IIqrfDpaService* iface) {
		if (m_iqrfDpaService == iface) {
			m_iqrfDpaService = nullptr;
		}
	}

	void IdeCounterpart::attachInterface(iqrf::IUdpMessagingService* iface) {
		m_messaging = iface;
	}

	void IdeCounterpart::detachInterface(iqrf::IUdpMessagingService* iface) {
		if (m_messaging == iface) {
			m_messaging = nullptr;
		}
	}

	void IdeCounterpart::attachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void IdeCounterpart::detachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
