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

#include "IMonitorService.h"
#include "ShapeProperties.h"
#include "IIqrfDb.h"
#include "IIqrfDpaService.h"
#include "IIqrfSensorData.h"
#include "IMessagingSplitterService.h"
#include "IUdpConnectorService.h"
#include "ILaunchService.h"
#include "ITraceService.h"
#include "Trace.h"
#include "WebsocketServer.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"

#include <condition_variable>
#include <string>
#include <thread>

/// iqrf namespace
namespace iqrf {

  /// Daemon monitoring service class
  class MonitorService : public IMonitorService {
  public:
    /**
     * Constructor
     */
    MonitorService();

    /**
     * Destructor
     */
    virtual ~MonitorService();

    /**
     * Get DPA message queue length
     * @return int Message queue length
     */
    int getDpaQueueLen() const override;

    /**
     * Get current IQRF channel state
     * @return IQRF channel state
     */
    virtual IIqrfChannelService::State getIqrfChannelState() override;

    /**
     * Get current DPA channel state
     * @return DPA channel state
     */
    virtual IIqrfDpaService::DpaState getDpaChannelState() override;

    /**
     * Wake up sleeping worker and force monitoring notification on demand
     */
    void invokeWorker() override;

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
     * Attaches DB service interface
     * @param iface DB service interface
     */
    void attachInterface(iqrf::IIqrfDb* iface);

    /**
     * Detaches DB service interface
     * @param iface DB srevice interface
     */
    void detachInterface(iqrf::IIqrfDb* iface);

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
     * Attaches sensor data service interface
     * @param iface Sensor data service interface
     */
    void attachInterface(iqrf::IIqrfSensorData* iface);

    /**
     * Detaches sensor data service interface
     * @param iface Sensor data service interface
     */
    void detachInterface(iqrf::IIqrfSensorData* iface);

    /**
     * Attaches splitter service interface
     * @param iface Splitter service interface
     */
    void attachInterface(iqrf::IMessagingSplitterService* iface);

    /**
     * Detaches splitter service interface
     * @param iface Splitter service interface
     */
    void detachInterface(iqrf::IMessagingSplitterService* iface);

    /**
     * Attaches UDP connector service interface
     * @param iface UDP connector service interface
     */
    void attachInterface(iqrf::IUdpConnectorService* iface);

    /**
     * Detaches UDP connector service interface
     * @param iface UDP connector service interface
     */
    void detachInterface(iqrf::IUdpConnectorService* iface);

    /**
     * Attaches launch service interface
     * @param iface Launch service interface
     */
    void attachInterface(shape::ILaunchService* iface);

    /**
     * Detaches launch service interface
     * @param iface Launch service interface
     */
    void detachInterface(shape::ILaunchService* iface);

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

    std::string getCertPath(const std::string& path);
    /**
     * Handles request from splitter
     * @param messaging Messaging instance
     * @param msgType Message type
     * @param doc request document
     */
    void handleMsg(const MessagingInstance& messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc);

    /**
     * Generates rapidjson document containing monitoring notification message
     * @return Monitoring notification message document
     */
    rapidjson::Document createMonitorMessage();

    /**
     * Notification worker thread
     */
    void worker();

    /// Instance ID
    std::string m_instanceId;
    // Launch service interface
    shape::ILaunchService *m_launchService = nullptr;
    /// Websocket server parameters
    WebsocketServerParams m_params;
    // DB service interface
    IIqrfDb *m_dbService = nullptr;
    /// DPA service interface
    IIqrfDpaService *m_dpaService = nullptr;
    /// Sensor data service interface
    IIqrfSensorData *m_sensorDataService = nullptr;
    /// Splitter service interface
    IMessagingSplitterService *m_splitterService = nullptr;
    /// UDP connector service
    IUdpConnectorService *m_udpConnectorService = nullptr;
    /// WebSocket server interface
    std::unique_ptr<WebsocketServer> m_server = nullptr;
    /// Monitoring notification worker thread
    std::thread m_workerThread;
    /// Thread running condition
    bool m_runThread = true;
    /// Worker mutex
    std::mutex m_workerMutex;
    /// Worker invocation mutex
    std::mutex m_invokeMutex;
    /// Condition variable
    std::condition_variable m_cv;
    /// Splitter message types filter
    const std::vector<std::string> m_mTypes = {
      "ntfDaemon_InvokeMonitor"
    };
    /// Notification period
    int m_reportPeriod = 20;
  };
}
