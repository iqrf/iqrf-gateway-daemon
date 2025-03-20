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

#include "ShapeProperties.h"
#include "IUdpConnectorService.h"
#include "ITraceService.h"
#include "IUdpMessagingService.h"
#include "IIqrfChannelService.h"
#include "IIqrfDpaService.h"
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
	class IdeCounterpart : public IUdpConnectorService {
	public:
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
		 * Set gateway mode
		 * @param mode Gateway mode
		 */
		void setMode(Mode mode) override;

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
		 * @param iface IQRF channel service interface
		 */
		void attachInterface(iqrf::IUdpMessagingService* iface);

		/**
		 * Detaches UDP messaging service interface
		 * @param iface IQRF channel service interface
		 */
		void detachInterface(iqrf::IUdpMessagingService* iface);

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
		/**
		 * Handles incoming UDP messages from IDE
		 * @param message IDE message
		 * @return Execution status code
		 */
		int handleMsg(const std::vector<uint8_t> &message);

		/**
		 * Validates UDP message
		 * @param message UDP message
		 */
		void validateMsg(const std::basic_string<unsigned char> &message);

		/**
		 * Asynchronous response handler, used for sending responses to IDE
		 * @param message Message to send
		 * @return Execution status code
		 */
		int sendMessageToIde(const std::basic_string<unsigned char> &message);

		/// IQRF channel service interface
		IIqrfChannelService *m_iqrfChannelService = nullptr;
		/// DPA service interface
		IIqrfDpaService *m_iqrfDpaService = nullptr;
		/// UDP messaging service interface
		IUdpMessagingService *m_messaging = nullptr;
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
	};
}
