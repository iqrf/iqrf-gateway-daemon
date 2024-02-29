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
#pragma once

#include "IIqrfDpaService.h"
#include "IIqrfDb.h"
#include "IIqrfNetworkEnum.h"
#include "IJsCacheService.h"
#include "IJsRenderService.h"
#include "ILaunchService.h"
#include "IMessagingSplitterService.h"
#include "ITraceService.h"
#include "ShapeProperties.h"
#include "Trace.h"

#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/rapidjson.h"

#include <sqlite_orm/sqlite_orm.h>

#include <atomic>
#include <cmath>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <set>

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#define EEEPROM_READ_MAX_LEN 54

typedef std::shared_ptr<iqrf::db::Product> ProductPtr;
typedef std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> UniqueProduct;
typedef std::tuple<uint16_t, uint16_t> HwpidTuple;
typedef std::tuple<uint16_t, std::string> OsTuple;
typedef std::tuple<iqrf::db::Device, ProductPtr> DeviceProduct;

static const int16_t PERIPHERAL_BINOUT = 75;
static const int16_t PERIPHERAL_DALI = 74;
static const int16_t PERIPHERAL_LIGHT = 113;
static const int16_t PERIPHERAL_SENSOR = 94;

namespace iqrf {

	/// IQRF Network Enumeration class
	class IqrfNetworkEnum : public IIqrfNetworkEnum {
	public:
		/**
		 * Constructor
		 */
		IqrfNetworkEnum();

		/**
		 * Destructor
		 */
		virtual ~IqrfNetworkEnum();

		/**
		 * Performs network enumeration according to request parameters
		 * @param parameters Enumeration parameters
		 */
		void enumerate(IIqrfNetworkEnum::EnumParams &parameters) override;

		/**
		 * Check if enumeration is in progress
		 * @return true if enumeration is in progress, false otherwise
		 */
		bool isRunning() override;

				/**
		 * Register enumeration handler
		 * @param clientId Handler owner
		 * @param handler Handler function
		 */
		void registerEnumerationHandler(const std::string &clientId, EnumerationHandler handler) override;

		/**
		 * Unregister enumeration handler
		 * @param clientId Handler owner
		 */
		void unregisterEnumerationHandler(const std::string &clientId) override;

		/**
		 * Reloads all drivers
		 */
		void reloadDrivers() override;

		/**
		 * Reloads coordinator drivers on-demand
		 */
		void reloadCoordinatorDrivers() override;

		/**
		 * Component instance lifecycle activate step
		 * @param props Component instance properties
		 */
		void activate(const shape::Properties *props = 0);

		/**
		 * Component instance lifecycle modify step
		 * @param props Component instance properties
		 */
		void modify(const shape::Properties *props);

		/**
		 * Component instance lifecycle deactivate step
		 */
		void deactivate();

		/**
		 * Attaches DB service interface
		 * @param iface DB service interface
		 */
		void attachInterface(IIqrfDb *iface);

		/**
		 * Detaches DB service interface
		 * @param iface DB service interface
		 */
		void detachInterface(IIqrfDb *iface);

		/**
		 * Attaches DPA service interface
		 * @param iface DPA service interface
		 */
		void attachInterface(IIqrfDpaService *iface);

		/**
		 * Detaches DPA service interface
		 * @param iface DPA service interface
		 */
		void detachInterface(IIqrfDpaService *iface);

		/**
		 * Attaches JS cache service interface
		 * @param iface JS cache service interface
		 */
		void attachInterface(IJsCacheService *iface);

		/**
		 * Detaches JS cache service interface
		 * @param iface JS cache service interface
		 */
		void detachInterface(IJsCacheService *iface);

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
		 * Attaches Launch service interface
		 * @param iface Launch service interface
		 */
		void attachInterface(shape::ILaunchService *iface);

		/**
		 * Detaches Launch service interface
		 * @param iface Launch service interface
		 */
		void detachInterface(shape::ILaunchService *iface);

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
		 * Attaches Tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService *iface);

		/**
		 * Dettaches Tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService *iface);

		/// Node memory address for FRC
		static const uint16_t m_memoryAddress = 0x04a0;
	private:
		/**
		 * Analyzes DPA responses and triggers enumeration if network altering action has been performed.
		 * @param message DPA response
		 */
		void analyzeDpaMessage(const DpaMessage &message);

		/**
		 * Runs enumeration
		 * @param parameters Enumeration parameters
		 */
		void runEnumeration(IIqrfNetworkEnum::EnumParams &parameters);

		/**
		 * Starts enumeration thread
		 * @param parameters Enumeration parameters
		 */
		void startEnumerationThread(IIqrfNetworkEnum::EnumParams &parameters);

		/**
		 * Stops enumeration thread
		 */
		void stopEnumerationThread();

		/**
		 * Compares network with database information, selects nodes to enumerate and retrieves their routing information
		 * @param reenumerate Force re-enumerate existing records in database
		 */
		void checkNetwork(bool reenumerate);

		/**
		 * Performs TR devices enumeration
		 */
		void enumerateDevices();

		/**
		 * Performs [C] device enumeration
		 */
		void coordinatorEnumeration();

		/**
		 * Perform enumeration by polling devices
		 */
		void pollEnumeration();

		/**
		 * Perform enumeration utilizing FRC commands
		 */
		void frcEnumeration();

		/**
		 * Sends EEEPROM read request and stores response data in a buffer
		 * @param data EEEPROM data buffer
		 * @param address EEEPROM address to read from
		 * @param len Number of bytes to read
		 */
		void eeepromRead(uint8_t* data, const uint16_t &address, const uint8_t &len);

		/**
		 * Performs FRC requests for device HWPID and HWPID version
		 * @param hwpidMap Map of device addresses and their respective HWPID and HWPID version
		 * @param frcCount Number of FRC requests to send
		 * @param nodes Number of nodes to read per FRC request
		 * @param remainingNodes Number of nodes to read in the final FRC request
		 */
		void frcHwpid(std::map<uint8_t, HwpidTuple> *hwpidMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes);

		/**
		 * Performs FRC requests for device DPA version
		 * @param dpaMap Map of device addresses and their respective DPA versions
		 * @param frcCount Number of FRC requests to send
		 * @param nodes Number of nodes to read per FRC request
		 * @param remainingNodes Number of nodes to read in the final FRC request
		 */
		void frcDpa(std::map<uint8_t, uint16_t> *dpaMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes);

		/**
		 * Performs FRC requests for device OS build and version
		 * @param osMap Map of device addresses and their respective OS builds and versions
		 * @param frcCount Number of FRC requests to send
		 * @param nodes Number of nodes to read per FRC request
		 * @param remainingNodes Number of nodes to read in the final FRC request
		 */
		void frcOs(std::map<uint8_t, OsTuple> *osMap, uint8_t &frcCount, uint8_t &nodes, uint8_t &remainingNodes);

		/**
		 * Performs FRC request to ping online nodes
		 * @return Set of online nodes addresses
		 */
		const std::set<uint8_t> frcPing();

		/**
		 * Performs FRC send selective and stores response data in a buffer
		 * @param data FRC data buffer
		 * @param address Memory address to read from
		 * @param pnum Peripheral number
		 * @param pcmd Peripheral command
		 * @param numNodes Number of nodes to read from
		 * @param processedNodes Number of already read nodes
		 */
		void frcSendSelectiveMemoryRead(uint8_t* data, const uint16_t &address, const uint8_t &pnum, const uint8_t &pcmd, const uint8_t &numNodes, const uint8_t &processedNodes);

		/**
		 * Performs FRC extra result request and stores response data in a buffer
		 * @param data FRC data buffer
		 */
		void frcExtraResult(uint8_t* data);

		/**
		 * Performs product package enumeration
		 */
		void productPackageEnumeration();

		/**
		 * Updates products in database
		 */
		void updateDatabaseProducts();

		/**
		 * Performs standard device enumeration
		 */
		void standardEnumeration();

		/**
		 * Performs binary output standard enumeration
		 * @param deviceId Device ID
		 * @param address Device address
		 */
		void binoutEnumeration(const uint32_t &deviceId, const uint8_t &address);

		/**
		 * Performs dali standard enumeration
		 * @param deviceId Device ID
		 * @param address Device address
		 */
		void daliEnumeration(const uint32_t &deviceId);

		/**
		 * Performs light standard enumeration
		 * @param deviceId Device ID
		 * @param address device address
		 */
		void lightEnumeration(const uint32_t &deviceId, const uint8_t &address);

		/**
		 * Performs sensor standard enumeration
		 * @param address Device address
		 */
		void sensorEnumeration(const uint8_t &address);

		/**
		 * Retrieves bonded devices
		 */
		void getBondedNodes();

		/**
		 * Retrieves discovered devices
		 * @param bondedNodes Vector of bonded nodes for filtering
		 */
		void getDiscoveredNodes();

		/**
		 * Retrieves MIDs of bonded devices
		 */
		void getMids();

		/**
		 * Retrieves VRN, zone and parent information
		 */
		void getRoutingInformation();

		/**
		 * Clears auxiliary buffers holding device information
		 */
		void clearAuxBuffers();

		/**
		 * Waits for and claims exclusive access when available
		 */
		void waitForExclusiveAccess();

		/**
		 * Resets exclusive access
		 */
		void resetExclusiveAccess();


		/**
		 * Loads default drivers based on coordinator information
		 */
		void loadCoordinatorDrivers();

		/**
		 * Loads drivers coresponding to the products
		 */
		void loadProductDrivers();

		/**
		 * Loads DaemonWrapper code
		 */
		std::string loadWrapper();

		std::vector<uint8_t> selectNodes(const std::set<uint8_t> &nodes, const uint8_t &idx, const uint8_t &count);

		void handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc);

		void sendEnumerationProgressMessage(IIqrfNetworkEnum::EnumerationProgress progress);

		/// Component instance name
		std::string m_instance;
		/// Path to daemon js wrapper
		std::string m_wrapperPath;
		/// DB service
		IIqrfDb *m_dbService = nullptr;
		/// DPA service
		IIqrfDpaService *m_dpaService = nullptr;
		/// Enumeration condition variable
		std::condition_variable m_exclusiveAccessCv;
		/// Exclusive access
		std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
		/// JS cache service
		IJsCacheService *m_cacheService = nullptr;
		/// JS render service
		IJsRenderService *m_renderService = nullptr;
		/// Launch service
		shape::ILaunchService *m_launchService = nullptr;
		/// Splitter service
		IMessagingSplitterService *m_splitterService = nullptr;
		/// Coordinator parameters
		IIqrfDpaService::CoordinatorParameters m_coordinatorParams;
		/// Set of device addresses to enumerate
		std::set<uint8_t> m_toEnumerate;
		/// Set of discovered device addresses
		std::set<uint8_t> m_discovered;
		/// Set of devices to delete
		std::set<uint8_t> m_toDelete;
		/// Map of device addresses and their respective MIDs
		std::map<uint8_t, uint32_t> m_mids;
		/// Map of device vrns
		std::map<uint8_t, uint8_t> m_vrns;
		/// Map of device zones
		std::map<uint8_t, uint8_t> m_zones;
		/// Map of device parent addresses
		std::map<uint8_t, uint8_t> m_parents;
		/// Map of unique products
		std::map<UniqueProduct, db::Product> m_productMap;
		/// Map of device addresses and their respective products
		std::map<uint8_t, ProductPtr> m_deviceProductMap;
		/// Enumerate network automatically without initial user invocation
		bool m_autoEnumerateBeforeInvoked = false;
		/// Enumerate network when daemon launches
		bool m_enumerateOnLaunch = false;
		/// Controls whether enumeration should run or stop
		std::atomic_bool m_enumRun;
		/// Repeat enumeration in case of a failure
		std::atomic_bool m_enumRepeat;
		/// Run enumeration thread
		std::atomic_bool m_enumThreadRun;
		/// Enumeration condition variable
		std::condition_variable m_enumCv;
		/// Enumeration mutex
		std::mutex m_enumMutex;
		/// Enumeration thread
		std::thread m_enumThread;
		/// Enumeration handler map
		std::map<std::string, EnumerationHandler> m_enumHandlers;
		/// Enumeration parameters
		EnumParams m_params;
		/// Enuemration message type
		std::string m_enumerateMsg = "iqrfNetworkEnum_Enumerate";
		/// Async enumeration response message type
		std::string m_enumerateAsyncMsg = "iqrfNetworkEnum_EnumerateAsync";
	};
}
