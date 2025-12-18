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

#define IModeService_EXPORTS

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

  ///// Component management /////

	void IdeCounterpart::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "IdeCounterpart instance activate" << std::endl
			<< "******************************"
		);
		modify(props);
    m_splitterService->registerFilteredMsgHandler(
      {
        MsgMode,
        MsgActivate,
        MsgDeactivate,
        MsgGatewayIdent,
        MsgTrInfo,
        MsgTrWrite,
      },
      [&](const MessagingInstance& messaging, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc) {
        handleSplitterMsg(messaging, msgType, std::move(doc));
      }
    );
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

		IModeService::Mode startupMode = IdeCounterpart::Mode::Operational;
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
		m_udpService = iface;
    m_udpService->registerMessageHandler([&](const MessagingInstance& messaging, const std::vector<uint8_t>& msg) {
			(void)messaging;
			return handleIdeMsg(msg);
		});
	}

	void IdeCounterpart::detachInterface(iqrf::IUdpMessagingService* iface) {
		if (m_udpService == iface) {
      m_udpService->unregisterMessageHandler();
			m_udpService = nullptr;
		}
	}

  void IdeCounterpart::attachInterface(iqrf::IApiTokenService* iface) {
		m_tokenService = iface;
	}

	void IdeCounterpart::detachInterface(iqrf::IApiTokenService* iface) {
		if (m_tokenService == iface) {
			m_tokenService = nullptr;
		}
	}

  void IdeCounterpart::attachInterface(iqrf::IMessagingSplitterService* iface) {
		m_splitterService = iface;
    m_splitterService->registerServiceModeCheck([this]() -> bool {
      return getMode() == Mode::Service;
    });
	}

	void IdeCounterpart::detachInterface(iqrf::IMessagingSplitterService* iface) {
		if (m_splitterService == iface) {
      m_splitterService->unregisterServiceModeCheck();
			m_splitterService = nullptr;
		}
	}

	void IdeCounterpart::attachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void IdeCounterpart::detachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().removeTracerService(iface);
	}

	///// API implementation /////

	IModeService::Mode IdeCounterpart::getMode() const {
		std::lock_guard<std::mutex> lck(m_modeMtx);
		return m_mode;
	}

  IModeService::ServiceModeType IdeCounterpart::getServiceModeType() const {
    std::lock_guard<std::mutex> lck(m_modeMtx);
    return m_modeType;
  }

	void IdeCounterpart::setMode(Mode mode) {
		TRC_FUNCTION_ENTER(NAME_PAR(mode, (int)mode));

		std::lock_guard<std::mutex> lck(m_modeMtx);

		if (mode == Mode::Operational) {
			m_exclusiveAcessor.reset();
			m_snifferAcessor.reset();
      // mark none mode type
      m_modeType = ServiceModeType::None;
		} else if (mode == Mode::Forwarding) {
			m_exclusiveAcessor.reset();
			m_snifferAcessor = m_iqrfChannelService->getAccess([&](const std::basic_string<unsigned char>& received)->int {
				return sendMessageToIde(received);
			}, IIqrfChannelService::AccesType::Sniffer);
      // mark none mode type
      m_modeType = ServiceModeType::None;
		} else if (mode == Mode::Service) {
			m_snifferAcessor.reset();
			m_exclusiveAcessor = m_iqrfChannelService->getAccess([&](const std::basic_string<unsigned char>& received)->int {
				return sendMessageToIde(received);
			}, IIqrfChannelService::AccesType::Exclusive);
      // mark legacy mode type
      m_modeType = ServiceModeType::Legacy;
		} else {
			return;
		}

		m_mode = mode;
    executeModeSetCallbacks();

		TRC_INFORMATION("Set mode " << ModeStringConvertor::enum2str(m_mode));
		TRC_FUNCTION_LEAVE("");
	}

	void IdeCounterpart::registerModeSetCallback(const std::string &instanceId, std::function<void()> callback) {
		std::lock_guard<std::mutex> lck(m_callbackMutex);
		m_setModeCallbacks.insert_or_assign(instanceId, callback);
	}

	void IdeCounterpart::unregisterModeSetCallback(const std::string &instanceId) {
		std::lock_guard<std::mutex> lck(m_callbackMutex);
		m_setModeCallbacks.erase(instanceId);
	}

  void IdeCounterpart::clientDisconnected(const MessagingInstance& messaging) {
    std::lock_guard<std::mutex> lock(m_modeMtx);
    // check if new service mode is active
    if (m_modeType != ServiceModeType::New) {
      return;
    }
    // check if disconnected client is websocket and has client session
    if (messaging.type != MessagingType::WS || !messaging.clientSession.has_value()) {
      return;
    }
    // check if disconnected session manages service mode
    auto owner = m_serviceModeOwner.value();
    auto session = owner.clientSession.value();
    auto candidate = messaging.clientSession.value();
    if (owner.instance != messaging.instance ||
      session.getSessionId() != candidate.getSessionId() ||
      session.getTokenId() != candidate.getTokenId()
    ) {
      return;
    }
    m_snifferAcessor.reset();
    m_exclusiveAcessor.reset();
    m_mode = Mode::Operational;
    m_modeType = ServiceModeType::None;
    m_serviceModeOwner.reset();
    // execute callbacks
    executeModeSetCallbacks();
  }

	///// Private methods /////

  const char* IdeCounterpart::statusCodeToString(StatusCode code) {
    switch (code) {
      case StatusCode::Ok:
        return "Ok";
      case StatusCode::InternalError:
        return "An unexpected internal error has occurred.";
      case StatusCode::WebSocketOnly:
        return "Service API is only available for WebSocket connections.";
      case StatusCode::InsufficientPermission:
        return "WebSocket API token does not have sufficient permissions to use Service API.";
      case StatusCode::AlreadyActive:
        return "Service mode has already been activated.";
      case StatusCode::AlreadyInactive:
        return "Service mode has already been deactivated.";
      case StatusCode::NotActive:
        return "Service mode is not active.";
      case StatusCode::LegacyActive:
        return "Legacy service mode is active.";
      case StatusCode::NotOwner:
        return "Another session manages service mode.";
      case StatusCode::WriteFailed:
        return "Failed to write data to TR.";
      default:
        return "Unknown error.";
    }
  }

  void IdeCounterpart::executeModeSetCallbacks() {
    std::lock_guard<std::mutex> lck(m_callbackMutex);
    for (auto &callback : m_setModeCallbacks) {
      if (callback.second) {
        callback.second();
      }
    }
  }

	int IdeCounterpart::handleIdeMsg(const std::vector<uint8_t> &msg) {
		TRC_DEBUG("\n==================================\n"
			<< "Received from UDP:\n"
			<< MEM_HEX_CHAR(msg.data(), msg.size())
		);
		const std::basic_string<uint8_t> message(msg.data(), msg.size());

		try {
			validateIdeMsg(message);
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
				params.IP = m_udpService->getListeningIpAddress();
				params.MAC = m_udpService->getListeningMacAddress();
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
			m_udpService->sendMessage(m_udpService->getMessagingInstance(), handler->getResponse());
			if (handler->isTrWriteRequired() && m_exclusiveAcessor) {
				m_exclusiveAcessor->send(data);
			}
			return 0;
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::logic_error, e, e.what());
			return -1;
		}
	}

	void IdeCounterpart::validateIdeMsg(const std::basic_string<unsigned char> &message) {
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
		SendTrData dataToSend(m_params.mode, message);
		dataToSend.buildResponse();
		m_udpService->sendMessage(m_udpService->getMessagingInstance(), dataToSend.getResponse());
		return 0;
	}

  void IdeCounterpart::handleSplitterMsg(const MessagingInstance& messaging, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc) {
    TRC_FUNCTION_ENTER(
      PAR(messaging.to_string())
      NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(patch, msgType.m_micro)
    )

    if (msgType.m_type == MsgMode) {
      legacyModeChange(doc, messaging);
      return;
    }

    StatusCode code = StatusCode::Ok;

    Document response(kObjectType);
    Pointer("/mType").Set(response, msgType.m_type);
    Pointer("/data/msgId").Set(response, Pointer("/data/msgId").Get(doc)->GetString());

    if (msgType.m_type == MsgActivate) {
      code = activateServiceMode(messaging);
    } else if (msgType.m_type == MsgDeactivate) {
      code = deactivateServiceMode(messaging);
    } else if (msgType.m_type == MsgGatewayIdent) {
      code = gatewayIdentification(response, messaging);
    } else if (msgType.m_type == MsgTrInfo) {
      code = transceiverInformation(response, messaging);
    } else if (msgType.m_type == MsgTrWrite) {
      code = trWrite(doc, messaging);
    }

    Pointer("/data/status").Set(response, static_cast<int>(code));
    Pointer("/data/statusStr").Set(response, statusCodeToString(code));
    m_splitterService->sendMessage(messaging, std::move(response));
    TRC_FUNCTION_LEAVE(PAR(static_cast<int>(code)));
  }

  void IdeCounterpart::legacyModeChange(rapidjson::Document& request, const MessagingInstance& messaging) {
    Document response(kObjectType);
    Pointer("/mType").Set(response, MsgMode);
    Pointer("/data/msgId").Set(response, Pointer("/data/msgId").Get(request)->GetString());
    bool verbose = Pointer("/data/returnVerbose").GetWithDefault(request, false).GetBool();
    Mode modeToSet = ModeStringConvertor::str2enum(Pointer("/data/req/operMode").Get(request)->GetString());
    Mode currentMode = getMode();

    try {
      if (m_udpService == nullptr) {
        throw std::logic_error("UDP service not active.");
      }

      if (m_modeType == ServiceModeType::New) {
        throw std::logic_error("WebSocket service mode is active.");
      }

      if(modeToSet != Mode::Unknown) {
        setMode(modeToSet);
      }

      currentMode = getMode();

      Pointer("/data/rsp/operMode").Set(response, ModeStringConvertor::enum2str(currentMode));
      Pointer("/data/status").Set(response, 0);
      if (verbose) {
        Pointer("/data/statusStr").Set(response, "ok");
      }
    } catch (const std::exception &e) {
      Pointer("/data/rsp/operMode").Set(response, ModeStringConvertor::enum2str(currentMode));
      Pointer("/data/errorStr").Set(response, e.what());
      Pointer("/data/status").Set(response, -1);
      if (verbose) {
        Pointer("/data/statusStr").Set(response, "err");
      }
    }
    m_splitterService->sendMessage(messaging, std::move(response));
  }

  IdeCounterpart::StatusCode IdeCounterpart::gatewayIdentification(rapidjson::Document& response, const MessagingInstance& messaging) {
    // check access and permissions
    if (messaging.type != MessagingType::WS || !messaging.clientSession.has_value()) {
      return StatusCode::WebSocketOnly;
    }
    auto sessionInfo = messaging.clientSession.value();
    auto token = m_tokenService->getApiToken(sessionInfo.getTokenId());
    if (!token->canUseServiceMode()) {
      return StatusCode::InsufficientPermission;
    }

    auto coordinatorParams = m_iqrfDpaService->getCoordinatorParameters();

    Pointer("/data/rsp/name").Set(response, m_params.name);
    Pointer("/data/rsp/hostname").Set(response, m_params.netBios);
    Pointer("/data/rsp/osBuild").Set(response, coordinatorParams.osBuildWord);
    Pointer("/data/rsp/osVersion").Set(response, coordinatorParams.osVersionByte);
    return StatusCode::Ok;
  }

  IdeCounterpart::StatusCode IdeCounterpart::transceiverInformation(rapidjson::Document& response, const MessagingInstance& messaging) {
    if (messaging.type != MessagingType::WS || !messaging.clientSession.has_value()) {
      return StatusCode::WebSocketOnly;
    }
    auto sessionInfo = messaging.clientSession.value();
    auto token = m_tokenService->getApiToken(sessionInfo.getTokenId());
    if (!token->canUseServiceMode()) {
      return StatusCode::InsufficientPermission;
    }

    auto coordinatorParams = m_iqrfDpaService->getCoordinatorParameters();

    Pointer("/data/rsp/mid").Set(response, coordinatorParams.mid);
    Pointer("/data/rsp/osVersion").Set(response, coordinatorParams.osVersionByte);
    Pointer("/data/rsp/osBuild").Set(response, coordinatorParams.osBuildWord);
    Pointer("/data/rsp/trMcuType").Set(response, coordinatorParams.trMcuType);
    return StatusCode::Ok;
  }

  IdeCounterpart::StatusCode IdeCounterpart::activateServiceMode(const MessagingInstance& messaging) {
    // check access and permissions
    if (messaging.type != MessagingType::WS || !messaging.clientSession.has_value()) {
      return StatusCode::WebSocketOnly;
    }
    auto sessionInfo = messaging.clientSession.value();
    auto token = m_tokenService->getApiToken(sessionInfo.getTokenId());
    if (!token->canUseServiceMode()) {
      return StatusCode::InsufficientPermission;
    }
    std::lock_guard<std::mutex> lock(m_modeMtx);
    // check if legacy service mode is active
    if (m_modeType == ServiceModeType::Legacy) {
      return StatusCode::LegacyActive;
    }
    // check if service mode enabled
    if (m_modeType == ServiceModeType::New) {
      return StatusCode::AlreadyActive;
    }
    // enable service mode
    m_snifferAcessor.reset();
    m_exclusiveAcessor = m_iqrfChannelService->getAccess([&](const std::basic_string<unsigned char>& received) {
      return sendJsonTrData(received);
    }, IIqrfChannelService::AccesType::Exclusive);
    m_modeType = ServiceModeType::New;
    m_mode = Mode::Service;
    m_serviceModeOwner.emplace(messaging);
    // execute callbacks
    executeModeSetCallbacks();
    return StatusCode::Ok;
  }

  IdeCounterpart::StatusCode IdeCounterpart::deactivateServiceMode(const MessagingInstance& messaging) {
    // check access and permissions
    if (messaging.type != MessagingType::WS || !messaging.clientSession.has_value()) {
      return StatusCode::WebSocketOnly;
    }
    auto sessionInfo = messaging.clientSession.value();
    auto token = m_tokenService->getApiToken(sessionInfo.getTokenId());
    if (!token->canUseServiceMode()) {
      return StatusCode::InsufficientPermission;
    }
    std::lock_guard<std::mutex> lock(m_modeMtx);
    // check if service mode is deactivated
    if (m_modeType == ServiceModeType::None) {
      return StatusCode::AlreadyInactive;
    }
    // check if legacy service mode is active
    if (m_modeType == ServiceModeType::Legacy) {
      return StatusCode::LegacyActive;
    }
    // check if this session manages service mode
    auto owner = m_serviceModeOwner.value();
    auto session = owner.clientSession.value();
    auto candidate = messaging.clientSession.value();
    if (owner.instance != messaging.instance ||
      session.getSessionId() != candidate.getSessionId() ||
      session.getTokenId() != candidate.getTokenId()
    ) {
      return StatusCode::NotOwner;
    }
    // deactivate service mode
    m_snifferAcessor.reset();
    m_exclusiveAcessor.reset();
    m_modeType = ServiceModeType::None;
    m_mode = Mode::Operational;
    m_serviceModeOwner.reset();
    // execute callbacks
    executeModeSetCallbacks();
    return StatusCode::Ok;
  }

  IdeCounterpart::StatusCode IdeCounterpart::trWrite(rapidjson::Document& request, const MessagingInstance& messaging) {
    // check access and permissions
    if (messaging.type != MessagingType::WS || !messaging.clientSession.has_value()) {
      return StatusCode::WebSocketOnly;
    }
    auto sessionInfo = messaging.clientSession.value();
    auto token = m_tokenService->getApiToken(sessionInfo.getTokenId());
    if (!token->canUseServiceMode()) {
      return StatusCode::InsufficientPermission;
    }
    // check if not in service mode
    if (m_modeType == ServiceModeType::None) {
      return StatusCode::NotActive;
    }
    // check if legacy service mode is active
    if (m_modeType == ServiceModeType::Legacy) {
      return StatusCode::LegacyActive;
    }
    // check if this session manages service mode
    auto owner = m_serviceModeOwner.value();
    auto session = owner.clientSession.value();
    auto candidate = messaging.clientSession.value();
    if (owner.instance != messaging.instance ||
      session.getSessionId() != candidate.getSessionId() ||
      session.getTokenId() != candidate.getTokenId()
    ) {
      return StatusCode::NotOwner;
    }
    auto packet = Pointer("/data/req/packet").Get(request)->GetArray();
    std::basic_string<uint8_t> data;
    data.reserve(packet.Size());
    for (const auto& byte : packet) {
      data.push_back(static_cast<uint8_t>(byte.GetUint()));
    }
    try {
      m_exclusiveAcessor->send(data);
    } catch (const std::exception &e) {
      TRC_WARNING("Failed to write data to TR: " << e.what());
      return StatusCode::WriteFailed;
    }
    return StatusCode::Ok;
  }

  int IdeCounterpart::sendJsonTrData(const std::basic_string<unsigned char>& trData) {
    Document rsp(kObjectType);
    Pointer("/mType").Set(rsp, MsgTrData);
    Pointer("/data/msgId").Set(rsp, "async");

    Document::AllocatorType& allocator = rsp.GetAllocator();
    Value arr(kArrayType);
    for (const uint8_t byte : trData) {
      arr.PushBack(byte, allocator);
    }

    Pointer("/data/rsp/packet").Set(rsp, arr, allocator);
    Pointer("/data/status").Set(rsp, static_cast<int>(StatusCode::Ok));
    Pointer("/data/statusStr").Set(rsp, statusCodeToString(StatusCode::Ok));

    try {
      m_splitterService->sendMessage(m_serviceModeOwner.value(), std::move(rsp));
    } catch (const std::exception &e) {
      TRC_WARNING("Failed to send message from network to client: " << e.what());
      return -1;
    }
    return 0;
  }

}
