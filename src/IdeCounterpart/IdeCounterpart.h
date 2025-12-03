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

#include "ShapeProperties.h"
#include "IModeService.h"
#include "IMessagingSplitterService.h"
#include "ITraceService.h"
#include "IUdpMessagingService.h"
#include "IIqrfChannelService.h"
#include "IIqrfDpaService.h"
#include "IApiTokenService.h"
#include "TaskQueue.h"
#include "Trace.h"
#include <string>

#include "crc.h"
#include "EnumStringConvertor.h"
#include "rapidjson/pointer.h"

#include "Commands/BaseCommand.h"
#include "Commands/GatewayIdentification.h"
#include "Commands/GatewayStatus.h"
#include "Commands/SendTrData.h"
#include "Commands/TrInfo.h"
#include "Commands/TrReset.h"
#include "Commands/TrWrite.h"
#include "Commands/UnknownCommand.h"

/// iqrf namespace
namespace iqrf {
	/// IDE counterpart class
	class IdeCounterpart : public IModeService {
	public:
		/**
		 * API status codes
		 */
		enum class StatusCode {
			Ok,                         //< No errors
			InternalError,              //< An unexpected internal error has occurred
			WebSocketOnly,              //< API message did not come from a websocket client
			InsufficientPermission,     //< API token lacks service mode permission
			AlreadyActive,              //< Attempt to activate already activated service mode
			AlreadyInactive,            //< Attempt to deactivete already inactive service mode
			NotActive,                  //< Service mode is not active, but required for the operation
			LegacyActive,               //< Legacy service mode is active, but operation requires new legacy mode
			NotOwner,                   //< Another session manages service mode
			WriteFailed,                //< Failed to write data to TR
		};

		/**
		 * Constructor
		 */
		IdeCounterpart();

		/**
		 * Destructor
		 */
		virtual ~IdeCounterpart();

		/**
		 * Get current gateway mode
		 * @return Gateway mode
		 */
		Mode getMode() const override;

		/**
		 * Get service mode type
		 * @return Service mode type
		 */
		ServiceModeType getServiceModeType() const override;

		/**
		 * Set gateway mode
		 * @param mode Gateway mode
		 */
		void setMode(Mode mode);

		/**
		 * Registers mode set callback
		 * @param instanceId Component instance
		 * @param callback Callable
		 */
		void registerModeSetCallback(const std::string &instanceId, std::function<void()> callback) override;

		/**
		 * Unregisters mode set callback
		 * @param instanceId Component instance
		 */
		void unregisterModeSetCallback(const std::string &instanceId) override;

		/**
		 * Checks if disconnected client owns service mode, and disables it
		 * @param messaging Messaging instance
		 */
		void clientDisconnected(const MessagingInstance& messaging) override;

		/**
		 * Initializes component
		 * @param props Component properties
		 */
		void activate(const shape::Properties *props = 0);

		/**
		 * Modifies component properties
		 * @param props Component properties
		 */
		void modify(const shape::Properties *props);

		/**
		 * Deactivates component
		 */
		void deactivate();

		/**
		 * Attaches IQRF channel service interface
		 * @param iface IQRF channel service interface
		 */
		void attachInterface(iqrf::IIqrfChannelService* iface);

		/**
		 * Detaches IQRF channel service interface
		 * @param iface IQRF channel service interface
		 */
		void detachInterface(iqrf::IIqrfChannelService* iface);

		/**
		 * Attaches DPA service interface
		 * @param iface DPA service interface
		 */
		void attachInterface(iqrf::IIqrfDpaService* iface);

		/**
		 * Detaches DPA service interface
		 * @param iface DPA service interface
		 */
		void detachInterface(iqrf::IIqrfDpaService* iface);

		/**
		 * Attaches UDP messaging service interface
		 * @param iface UDP messaging service interface
		 */
		void attachInterface(iqrf::IUdpMessagingService* iface);

		/**
		 * Detaches UDP messaging service interface
		 * @param iface UDP messaging service interface
		 */
		void detachInterface(iqrf::IUdpMessagingService* iface);

		/**
		 * Attaches API token service interface
		 * @param iface API token service interface
		 */
		void attachInterface(iqrf::IApiTokenService* iface);

		/**
		 * Detaches API token service interface
		 * @param iface API token service interface
		 */
		void detachInterface(iqrf::IApiTokenService* iface);

		/**
		 * Attaches Splitter service interface
		 * @param iface Splitter service interface
		 */
		void attachInterface(iqrf::IMessagingSplitterService* iface);

		/**
		 * Detaches Splitter service interface
		 * @param iface Splitter service interface
		 */
		void detachInterface(iqrf::IMessagingSplitterService* iface);

		/**
		 * Attaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService* iface);

		/**
		 * Detaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService* iface);
	private:
		/// Supported message types

		static constexpr const char* MsgMode = "mngDaemon_Mode";
		static constexpr const char* MsgActivate = "mngService_Activate";
		static constexpr const char* MsgDeactivate = "mngService_Deactivate";
		static constexpr const char* MsgGatewayIdent = "mngService_GwIdentification";
		static constexpr const char* MsgTrInfo = "mngService_TrInfo";
		static constexpr const char* MsgTrWrite = "mngService_TrWrite";
		static constexpr const char* MsgTrData = "mngService_TrData";

		/// IQRF channel service interface
		IIqrfChannelService *m_iqrfChannelService = nullptr;
		/// DPA service interface
		IIqrfDpaService *m_iqrfDpaService = nullptr;
		/// UDP messaging service interface
		IUdpMessagingService *m_udpService = nullptr;
		/// API token service interface
		IApiTokenService *m_tokenService = nullptr;
		/// Splitter service interface
		IMessagingSplitterService *m_splitterService = nullptr;
		/// Gateway mode mutex
		mutable std::mutex m_modeMtx;
		/// Current gateway mode
		Mode m_mode;
		/// Acessors
		std::unique_ptr<IIqrfChannelService::Accessor> m_exclusiveAcessor;
		std::unique_ptr<IIqrfChannelService::Accessor> m_snifferAcessor;
		/// Gateway identification parameters
		GwIdentParams m_params {0x20, "iqrf-gateway-daemon", "N/A", "N/A", "N/A", "N/A", "N/A"};
		/// Callback mutex
		mutable std::mutex m_callbackMutex;
		/// Map of Mode set callbacks
		std::map<std::string, std::function<void()>> m_setModeCallbacks;
		/// Mode type
		ServiceModeType m_modeType = ServiceModeType::None;
		/// Service mode owner
		std::optional<MessagingInstance> m_serviceModeOwner;

		/**
		 * Convert status code to status message
		 * @param code Status code
		 * @return Status message
		 */
		static const char* statusCodeToString(StatusCode code);

		/**
		 * Executes registered mode set callbacks
		 */
		void executeModeSetCallbacks();

		/**
		 * Handles incoming UDP messages from IDE
		 * @param message IDE message
		 * @return Execution status code
		 */
		int handleIdeMsg(const std::vector<uint8_t> &message);

		/**
		 * Validates UDP message from IDE
		 * @param message UDP message
		 */
		void validateIdeMsg(const std::basic_string<unsigned char>& message);

		/**
		 * Sends asynchronous confirmation and response packets via UDP
		 * @param message Message to send
		 * @return Execution status code
		 */
		int sendMessageToIde(const std::basic_string<unsigned char>& message);

		/**
		 * Handles incoming JSON API messages
		 * @param messaging Messaging instance
		 * @param msgType Message information
		 * @param doc Document containing message
		 */
		void handleSplitterMsg(const MessagingInstance& messaging, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc);

		/**
		 * Handles legacy mode change request
		 * @param request Request document
		 * @param messaging Messaging instance
		 */
		void legacyModeChange(rapidjson::Document& request, const MessagingInstance& messaging);

		/**
		 * Handles activate service mode message
		 * @param messaging Messaging instance
		 * @return Status code
		 */
		StatusCode activateServiceMode(const MessagingInstance& messaging);

		/**
		 * Handles deactivate service mode message
		 * @param messaging Messaging instance
		 * @return Status code
		 */
		StatusCode deactivateServiceMode(const MessagingInstance& messaging);

		/**
		 * Handles gateway identification message
		 * @param messaging Messaging instance
		 * @return Status code
		 */
		StatusCode gatewayIdentification(rapidjson::Document& response, const MessagingInstance& messaging);

		/**
		 * Handles transceiver information message
		 * @param messaging Messaging instance
		 * @return Status code
		 */
		StatusCode transceiverInformation(rapidjson::Document& response, const MessagingInstance& messaging);

		/**
		 * Handles write data to tr message
		 * @param messaging Messaging instance
		 * @return Status code
		 */
		StatusCode trWrite(rapidjson::Document& request, const MessagingInstance& messaging);

		/**
		 * Sends asynchronous confirmation and response packets via WebSocket
		 * @param message Message to send
		 * @return Execution status code
		 */
		int sendJsonTrData(const std::basic_string<unsigned char>& message);
	};
}
