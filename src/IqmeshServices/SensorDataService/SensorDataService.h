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
#pragma once

#include "IConfigurationService.h"
#include "IIqrfDb.h"
#include "IIqrfDpaService.h"
#include "IJsRenderService.h"
#include "IMessagingSplitterService.h"
#include "ISensorDataService.h"
#include "ITraceService.h"
#include "SensorDataResult.h"
#include "ShapeProperties.h"
#include "JsDriverFrc.h"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <thread>

#define FRC_CMD_1BYTE 0x90
#define FRC_CMD_2BYTE 0xE0
#define FRC_CMD_4BYTE 0xF9

namespace iqrf {
	/// Sensor data service class
	class SensorDataService : public ISensorDataService {
	public:
		/// Service error codes
		enum ErrorCodes {
			serviceError = 1000,
			requestParseError = 1001,
			exclusiveAccessError = 1002,
			notRunning = 1003,
			readingInProgress = 1004,
			configNotFound = 1005,
		};

		/**
		 * Constructor
		 */
		SensorDataService();

		/**
		 * Destructor
		 */
		virtual ~SensorDataService();

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
		 * Attaches configuration service interface
		 * @param iface Configuration service interface
		 */
		void attachInterface(shape::IConfigurationService *iface);

		/**
		 * Detaches configuration service interface
		 * @param iface Configuration service interface
		 */
		void detachInterface(shape::IConfigurationService *iface);

		/**
		 * Attaches DB service interface
		 * @param iface DB service interface
		 */
		void attachInterface(iqrf::IIqrfDb *iface);

		/**
		 * Detaches DB service interface
		 * @param iface DB service interface
		 */
		void detachInterface(iqrf::IIqrfDb *iface);

		/**
		 * Attaches DPA service interface
		 * @param iface DPA service interface
		 */
		void attachInterface(iqrf::IIqrfDpaService *iface);

		/**
		 * Detaches DPA service interface
		 * @param iface DPA service interface
		 */
		void detachInterface(iqrf::IIqrfDpaService *iface);

		/**
		 * Attaches JS render service interface
		 * @param iface JS render service interface
		 */
		void attachInterface(IJsRenderService *iface);

		/**
		 * Detaches JS render service interface
		 * @param iface JS render service interface
		 */
		void detachInterface(IJsRenderService *iface);

		/**
		 * Attaches splitter service interface
		 * @param iface Splitter service interface
		 */
		void attachInterface(IMessagingSplitterService *iface);

		/**
		 * Detaches splitter service interface
		 * @param iface Splitter service interface
		 */
		void detachInterface(IMessagingSplitterService *iface);

		/**
		 * Attaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService *iface);

		/**
		 * Detaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService *iface);
	private:
		/**
		 * Set transaction result, error code and error string
		 * @param result Service result
		 * @param transResult Transaction result
		 * @param errorStr Error string
		 */
		void setErrorTransactionResult(SensorDataResult &result, std::unique_ptr<IDpaTransactionResult2> &transResult, const std::string &errorStr);

		/**
		 * Returns number of devices that data can be collected from in one request by sensor type
		 * @param type Sensor type
		 * @return Number of devices per request
		 */
		uint8_t frcDeviceCountByType(const uint8_t &type);

		/**
		 * Checks if extra result request is required to collect data from all devices
		 * @param command FRC command
		 * @param deviceCount Number of devices
		 * @return True if device count requires extra result request to retrieve all data, false otherwise
		 */
		bool extraResultRequired(const uint8_t &command, const uint8_t &deviceCount);

		/**
		 * Split set of into vector of sets of specific size
		 * @param set Set to split
		 * @param size Sub-set size
		 * @return std::vector<std::set<uint8_t>> Subsets
		 */
		std::vector<std::set<uint8_t>> splitSet(std::set<uint8_t> &set, size_t size);

		/**
		 * Sets offline FRC flag
		 * @param result Service result
		 */
		void setOfflineFrc(SensorDataResult &result);

		/**
		 * Sends Sensor FRC and stores response data
		 * @param result Service result
		 * @param type Sensor type
		 * @param idx Sensor index
		 * @param nodes Devices to read data from
		 */
		std::vector<iqrf::sensor::item::Sensor> sendSensorFrc(SensorDataResult &result, const uint8_t &type, const uint8_t &idx, std::set<uint8_t> &nodes);

		/**
		 * Prepares requests for specified sensor type, index and selected devices
		 * @param result Service result
		 * @param type Sensor type
		 * @param idx Sensor index
		 * @param addresses Devices to read data from
		 */
		void getTypeData(SensorDataResult &result, const uint8_t &type, const uint8_t &idx, std::deque<uint8_t> &addresses);

		/**
		 * Set device HWPID and MID by node addresses
		 * @param result Service result
		 * @param nodes Nodes
		 */
		void setDeviceHwpidMid(SensorDataResult &result, std::set<uint8_t> &nodes);

		/**
		 * Get RSSI from regular devices
		 * @param result Service result
		 * @param nodes Nodes to get RSSI from
		 */
		void getRssi(SensorDataResult &result, std::set<uint8_t> &nodes);

		/**
		 * Get RSSI from beaming devices
		 * @param result Service result
		 * @param nodes Nodes to get RSSI from
		 */
		void getRssiBeaming(SensorDataResult &result, std::set<uint8_t> &nodes);

		/**
		 * Reads Sensor data using FRC requests
		 * @param result Service result
		 */
		void getDataByFrc(SensorDataResult &result);

		/**
		 * Sensor data reading worker
		 */
		void worker();

		/**
		 * Notifies a sleeping worker
		 * @param request Request document
		 * @param messagingId Messaging ID
		 */
		void notifyWorker(rapidjson::Document &request, const std::string &messagingId);

		/**
		 * Starts the worker
		 * @param request Request document
		 * @param messagingId Messaging ID
		 */
		void startWorker(rapidjson::Document &request, const std::string &messagingId);

		/**
		 * Stops the worker
		 * @param request Request document
		 * @param messagingId Messaging ID
		 */
		void stopWorker(rapidjson::Document &request, const std::string &messagingId);

		/**
		 * Returns worker configuration
		 * @param request Request document
		 * @param messagingId Messaging ID
		 */
		void getConfig(rapidjson::Document &request, const std::string &messagingId);

		/**
		 * Configures worker
		 * @param request Request document
		 * @param messagingId Messaging ID
		 */
		void setConfig(rapidjson::Document &request, const std::string &messagingId);

		/**
		 * Handles request from splitter
		 * @param messagingId Messaging ID
		 * @param msgType Message type
		 * @param doc request document
		 */
		void handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc);

		/// Component name
		std::string m_componentName;
		/// Instance name
		std::string m_instanceName;
		/// Configuratiion service
		shape::IConfigurationService *m_configService = nullptr;
		/// DB service
		IIqrfDb *m_dbService = nullptr;
		/// DPA service
		IIqrfDpaService *m_dpaService = nullptr;
		/// JS render service
		IJsRenderService *m_jsRenderService = nullptr;
		/// Splitter service
		IMessagingSplitterService *m_splitterService = nullptr;
		/// Exclusive access
		std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
		/// Worker thread
		std::thread m_workerThread;
		/// Worker thread run condition
		bool m_workerRun = false;
		/// Mutex
		std::mutex m_mtx;
		/// Condition variable
		std::condition_variable m_cv;
		/// Run worker thread at start
		bool m_autoRun = false;
		/// Reading execution period
		uint32_t m_period = 1;
		/// Async reports
		bool m_asyncReports = false;
		/// Async response messaging list
		std::list<std::string> m_messagingList;
		/// Handled API request mtype
		const std::string m_messageType = "iqmeshNetwork_SensorData";
		/// Async response message type
		const std::string m_messageTypeAsync = "iqmeshNetwork_SensorDataAsync";
		/// Input parameters
		TSensorDataInputParams m_params;
	};
}
