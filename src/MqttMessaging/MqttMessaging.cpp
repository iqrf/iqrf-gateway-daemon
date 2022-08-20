/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

#define IMessagingService_EXPORTS

#include "MqttMessaging.h"
#include "TaskQueue.h"
#include "MQTTAsync.h"
#include <atomic>
#include <future>

#define CONNECTION(broker, client)  "[" << broker << ":" << client << "]: "

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "Trace.h"

#include "iqrf__MqttMessaging.hxx"

TRC_INIT_MODULE(iqrf::MqttMessaging);

namespace iqrf {

  typedef std::basic_string<uint8_t> ustring;

  class MqttMessagingImpl {

  private:
    //configuration
    std::string m_mqttBrokerAddr;
    std::string m_mqttClientId;
    int m_mqttPersistence = 0;
    std::string m_mqttTopicRequest;
    std::string m_mqttTopicResponse;
    int m_mqttQos = 0;
    std::string m_mqttUser;
    std::string m_mqttPassword;
    bool m_mqttEnabledSSL = false;
    //special msg sent to keep connection alive
    int m_mqttKeepAliveInterval = 20;
    //waits for accept from broker side
    int m_mqttConnectTimeout = 5;
    //waits to reconnect when connection broken
    int m_mqttMinReconnect = 1;
    //waits time *= 2 with every unsuccessful attempt up to this value
    int m_mqttMaxReconnect = 64;
    //The file in PEM format containing the public digital certificates trusted by the client.
    std::string m_trustStore;
    //The file in PEM format containing the public certificate chain of the client. It may also include
    //the client's private key.
    std::string m_keyStore;
    //If not included in the sslKeyStore, this setting points to the file in PEM format containing
    //the client's private key.
    std::string m_privateKey;
    //The password to load the client's privateKey if encrypted.
    std::string m_privateKeyPassword;
    //The list of cipher suites that the client will present to the server during the SSL handshake.For a
    //full explanation of the cipher list format, please see the OpenSSL on - line documentation :
    //http ://www.openssl.org/docs/apps/ciphers.html#CIPHER_LIST_FORMAT
    std::string m_enabledCipherSuites;
    //True/False option to enable verification of the server certificate
    bool m_enableServerCertAuth = true;

    std::string m_name;
    bool m_acceptAsyncMsg = false;

    TaskQueue<ustring>* m_toMqttMessageQueue = nullptr;
    IMessagingService::MessageHandlerFunc m_messageHandlerFunc;

    MQTTAsync m_client = nullptr;

    std::atomic<MQTTAsync_token> m_deliveredtoken;
    std::atomic_bool m_connected;
    std::atomic_bool m_subscribed;

    MQTTAsync_connectOptions m_conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_SSLOptions m_ssl_opts = MQTTAsync_SSLOptions_initializer;
    MQTTAsync_disconnectOptions m_disc_opts = MQTTAsync_disconnectOptions_initializer;
    MQTTAsync_responseOptions m_subs_opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_responseOptions m_send_opts = MQTTAsync_responseOptions_initializer;

    std::mutex m_connectionMutex;

    std::promise<bool> m_disconnect_promise;
    std::future<bool> m_disconnect_future = m_disconnect_promise.get_future();

  public:
    //------------------------
    MqttMessagingImpl()
      : m_toMqttMessageQueue(nullptr)
    {
      m_connected = false;
    }

    //------------------------
    ~MqttMessagingImpl()
    {}

    //------------------------
    void update(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");

      props->getMemberAsString("instance", m_name);
      props->getMemberAsString("BrokerAddr", m_mqttBrokerAddr);
      props->getMemberAsString("ClientId", m_mqttClientId);
      props->getMemberAsInt("Persistence", m_mqttPersistence);
      props->getMemberAsInt("Qos", m_mqttQos);
      props->getMemberAsString("TopicRequest", m_mqttTopicRequest);
      props->getMemberAsString("TopicResponse", m_mqttTopicResponse);
      props->getMemberAsString("User", m_mqttUser);
      props->getMemberAsString("Password", m_mqttPassword);
      props->getMemberAsBool("EnabledSSL", m_mqttEnabledSSL);

      props->getMemberAsString("TrustStore", m_trustStore);
      props->getMemberAsString("KeyStore", m_keyStore);
      props->getMemberAsString("PrivateKey", m_privateKey);
      props->getMemberAsString("PrivateKeyPassword", m_privateKeyPassword);
      props->getMemberAsString("EnabledCipherSuites", m_enabledCipherSuites);
      props->getMemberAsBool("EnableServerCertAuth", m_enableServerCertAuth);

      props->getMemberAsInt("KeepAliveInterval", m_mqttKeepAliveInterval);
      props->getMemberAsInt("ConnectTimeout", m_mqttConnectTimeout);
      props->getMemberAsInt("MinReconnect", m_mqttMinReconnect);
      props->getMemberAsInt("MaxReconnect", m_mqttMaxReconnect);
      props->getMemberAsBool("acceptAsyncMsg", m_acceptAsyncMsg);

      TRC_FUNCTION_LEAVE("");
    }

    //------------------------
    void start()
    {
      TRC_FUNCTION_ENTER("");

      m_toMqttMessageQueue = shape_new TaskQueue<ustring>([&](const ustring& msg) {
        sendTo(msg);
      });

      m_ssl_opts.enableServerCertAuth = true;

      if (!m_trustStore.empty()) m_ssl_opts.trustStore = m_trustStore.c_str();
      if (!m_keyStore.empty()) m_ssl_opts.keyStore = m_keyStore.c_str();
      if (!m_privateKey.empty()) m_ssl_opts.privateKey = m_privateKey.c_str();
      if (!m_privateKeyPassword.empty()) m_ssl_opts.privateKeyPassword = m_privateKeyPassword.c_str();
      if (!m_enabledCipherSuites.empty()) m_ssl_opts.enabledCipherSuites = m_enabledCipherSuites.c_str();
      m_ssl_opts.enableServerCertAuth = m_enableServerCertAuth;

      int retval;

      if ((retval = MQTTAsync_create(&m_client, m_mqttBrokerAddr.c_str(),
        m_mqttClientId.c_str(), m_mqttPersistence, NULL)) != MQTTASYNC_SUCCESS) {
        THROW_EXC_TRC_WAR(
          std::logic_error,
          CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
          "MQTTClient_create() failed: " <<
          PAR(retval)
        );
      }

      int ret = MQTTAsync_setConnected(m_client, this, s_connected);
      if (ret != MQTTASYNC_SUCCESS) {
        THROW_EXC_TRC_WAR(
          std::logic_error,
          CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
          "Failed to set reconnect callback." << PAR(ret)
        );
      }

      m_conn_opts.keepAliveInterval = m_mqttKeepAliveInterval;
      m_conn_opts.cleansession = 1;
      m_conn_opts.connectTimeout = m_mqttConnectTimeout;
      m_conn_opts.automaticReconnect = 1;
      m_conn_opts.minRetryInterval = m_mqttMinReconnect;
      m_conn_opts.maxRetryInterval = m_mqttMaxReconnect;
      m_conn_opts.username = m_mqttUser.c_str();
      m_conn_opts.password = m_mqttPassword.c_str();
      m_conn_opts.onSuccess = s_connectSuccess;
      m_conn_opts.onFailure = s_connectFailed;
      m_conn_opts.context = this;

      m_subs_opts.onSuccess = s_onSubscribe;
      m_subs_opts.onFailure = s_onSubscribeFailure;
      m_subs_opts.context = this;

      m_send_opts.onSuccess = s_onSend;
      m_send_opts.onFailure = s_onSendFailure;
      m_send_opts.context = this;

      if (m_mqttEnabledSSL) {
        m_conn_opts.ssl = &m_ssl_opts;
      }

      ret = MQTTAsync_setCallbacks(m_client, this, s_connlost, s_msgarrvd, s_delivered);
      if (ret != MQTTASYNC_SUCCESS) {
        THROW_EXC_TRC_WAR(
          std::logic_error,
          CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
          "MQTTClient_setCallbacks() failed: " <<
          PAR(retval)
        );
      }

      TRC_INFORMATION("daemon-MQTT-protocol started - trying to connect broker: " << m_mqttBrokerAddr);

      connect();

      TRC_FUNCTION_LEAVE("");
    }

    //------------------------
    void stop()
    {
      TRC_FUNCTION_ENTER("");

      int retval;
      m_disc_opts.onSuccess = s_onDisconnect;
      m_disc_opts.context = this;
      if ((retval = MQTTAsync_disconnect(m_client, &m_disc_opts)) != MQTTASYNC_SUCCESS) {
        TRC_WARNING(
          CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
          "Failed to start disconnect: " <<
          PAR(retval)
        );
        onDisconnect(nullptr);
      }

      //wait for async disconnect
      std::chrono::milliseconds span(5000);
      if (m_disconnect_future.wait_for(span) == std::future_status::timeout) {
        TRC_WARNING("Timeout to wait disconnect");
      }

      MQTTAsync_setCallbacks(m_client, nullptr, nullptr, nullptr, nullptr);

      MQTTAsync_destroy(&m_client);
      delete m_toMqttMessageQueue;

      TRC_INFORMATION("daemon-MQTT-protocol stopped");

      TRC_FUNCTION_LEAVE("");
    }

    //------------------------
    const std::string& getName() const { return m_name; }

    bool acceptAsyncMsg() const
    {
      return m_acceptAsyncMsg;
    }

    //------------------------
    void registerMessageHandler(IMessagingService::MessageHandlerFunc hndl) {
      m_messageHandlerFunc = hndl;
    }

    //------------------------
    void unregisterMessageHandler() {
      m_messageHandlerFunc = IMessagingService::MessageHandlerFunc();
    }

    //------------------------
    void sendMessage(const ustring& msg) {
      m_toMqttMessageQueue->pushToQueue(msg);
    }

    //------------------------
    void handleMessageFromMqtt(const ustring& message)
    {
      TRC_DEBUG("==================================" << std::endl <<
        "Received from MQTT: " << std::endl << MEM_HEX_CHAR(message.data(), message.size()));

      if (m_messageHandlerFunc)
        m_messageHandlerFunc(m_name, std::vector<uint8_t>(message.data(), message.data() + message.size()));
    }

    //------------------------
    void sendTo(const ustring& msg)
    {
      TRC_DEBUG("Sending to MQTT: " << NAME_PAR(topic, m_mqttTopicResponse) << std::endl <<
        MEM_HEX_CHAR(msg.data(), msg.size()));

      if (m_connected) {

        int retval;
        MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

        pubmsg.payload = (void*)msg.data();
        pubmsg.payloadlen = (int)msg.size();
        pubmsg.qos = m_mqttQos;
        pubmsg.retained = 0;

        m_deliveredtoken = 0;

        if ((retval = MQTTAsync_sendMessage(m_client, m_mqttTopicResponse.c_str(), &pubmsg, &m_send_opts)) != MQTTASYNC_SUCCESS)
        {
          TRC_WARNING(
            CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
            "Failed to start sendMessage: " <<
            PAR(retval)
          );
        }

      }
      else {
        TRC_WARNING(CONNECTION(m_mqttBrokerAddr, m_mqttClientId) << "Cannot send message to, client not connected.");
      }
    }

    //------------------------
    void connect()
    {
      TRC_FUNCTION_ENTER("");

      m_connected = false;
      m_subscribed = false;

      int ret = MQTTAsync_connect(m_client, &m_conn_opts);
      if (ret != MQTTASYNC_SUCCESS) {
        TRC_WARNING(
          CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
          "MQTTASync_connect() failed: " <<
          PAR(ret)
        );
      }

      TRC_FUNCTION_LEAVE("");
    }


    //------------------------
    static void s_connectSuccess(void* context, MQTTAsync_successData* response) {
      ((MqttMessagingImpl*)context)->connectSuccessCallback(response);
    }
    void connectSuccessCallback(MQTTAsync_successData* response) {
      MQTTAsync_token token = 0;
      char* suri = nullptr;
      std::string serverUri;
      int MQTTVersion = 0;
      int sessionPresent = 0;

      if (response) {
        token = response->token;
        suri = response->alt.connect.serverURI;
        serverUri = suri ? suri : "";
        MQTTVersion = response->alt.connect.MQTTVersion;
        sessionPresent = response->alt.connect.sessionPresent;
      }

      TRC_INFORMATION(
        CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
        "Connect succeeded: " <<
        PAR(token) <<
        PAR(serverUri) <<
        PAR(MQTTVersion) <<
        PAR(sessionPresent)
      );

      {
        std::unique_lock<std::mutex> lck(m_connectionMutex);
        m_connected = true;
      }
    }

    //------------------------
    static void s_connectFailed(void* context, MQTTAsync_failureData* response) {
      ((MqttMessagingImpl*)context)->connectFailedCallback(response);
    }
    void connectFailedCallback(MQTTAsync_failureData* response) {
      TRC_FUNCTION_ENTER("");
      if (response) {
        TRC_WARNING(
          CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
          "Connect failed: " <<
          PAR(response->code) <<
          NAME_PAR(errmsg, (response->message ? response->message : "-")) <<
          PAR(m_mqttTopicRequest) <<
          PAR(m_mqttQos)
        );
      }

      {
        std::unique_lock<std::mutex> lck(m_connectionMutex);
        m_connected = false;
      }

      TRC_FUNCTION_LEAVE("");
    }

    //------------------------
    static void s_connected(void *context, char *cause) {
      ((MqttMessagingImpl *)context)->connected(cause);
    }
    void connected(char *cause) {
      (void)cause;
      TRC_INFORMATION(CONNECTION(m_mqttBrokerAddr, m_mqttClientId) << "(Re-)connect success.");

      {
        std::unique_lock<std::mutex> lck(m_connectionMutex);
        m_connected = true;
      }

      TRC_DEBUG(
        CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
        "Subscribing: " << PAR(m_mqttTopicRequest) << PAR(m_mqttQos)
      );
      int ret = MQTTAsync_subscribe(m_client, m_mqttTopicRequest.c_str(), m_mqttQos, &m_subs_opts);
      if (ret != MQTTASYNC_SUCCESS) {
        TRC_WARNING(
          CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
          "MQTTAsync_subscribe() failed: " <<
          PAR(ret) <<
          PAR(m_mqttTopicRequest) <<
          PAR(m_mqttQos)
        );
      }
    }

    //------------------------
    static void s_onSubscribe(void* context, MQTTAsync_successData* response) {
      ((MqttMessagingImpl*)context)->onSubscribe(response);
    }
    void onSubscribe(MQTTAsync_successData* response) {
      MQTTAsync_token token = 0;
      int qos = 0;

      if (response) {
        token = response->token;
        qos = response->alt.qos;
      }

      TRC_INFORMATION(
        CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
        "Subscribe succeeded: " <<
        PAR(m_mqttTopicRequest) <<
        PAR(m_mqttQos) <<
        PAR(token) <<
        PAR(qos)
      );
      m_subscribed = true;
    }

    //------------------------
    static void s_onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
      ((MqttMessagingImpl*)context)->onSubscribeFailure(response);
    }
    void onSubscribeFailure(MQTTAsync_failureData* response) {

      MQTTAsync_token token = 0;
      int code = 0;
      std::string message;

      if (response) {
        token = response->token;
        code = response->code;
        message = response->message ? response->message : "";
      }

      TRC_WARNING(
        CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
        "Subscribe failed: " <<
        PAR(m_mqttTopicRequest) <<
        PAR(m_mqttQos) <<
        PAR(token) <<
        PAR(code) <<
        PAR(message)
      );
      m_subscribed = false;
    }

    //------------------------
    static void s_delivered(void *context, MQTTAsync_token dt) {
      ((MqttMessagingImpl*)context)->delivered(dt);
    }
    void delivered(MQTTAsync_token dt) {
      TRC_DEBUG(
        CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
        "Message delivery confirmed" << PAR(dt)
      );
      m_deliveredtoken = dt;
    }

    //------------------------
    static int s_msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
      return ((MqttMessagingImpl*)context)->msgarrvd(topicName, topicLen, message);
    }
    int msgarrvd(char *topicName, int topicLen, MQTTAsync_message *message) {
      ustring msg((unsigned char*)message->payload, message->payloadlen);
      std::string topic;
      if (topicLen > 0)
        topic = std::string(topicName, topicLen);
      else
        topic = std::string(topicName);
      //TODO wildcards in comparison - only # supported now
      TRC_DEBUG(PAR(topic));
      size_t sz = m_mqttTopicRequest.size();
      if (m_mqttTopicRequest[--sz] == '#') {
        if (0 == m_mqttTopicRequest.compare(0, sz, topic, 0, sz))
          handleMessageFromMqtt(msg);
      }
      else if (0 == m_mqttTopicRequest.compare(topic))
        handleMessageFromMqtt(msg);
      MQTTAsync_freeMessage(&message);
      MQTTAsync_free(topicName);
      return 1;
    }

    //------------------------
    static void s_onSend(void* context, MQTTAsync_successData* response) {
      ((MqttMessagingImpl*)context)->onSend(response);
    }
    void onSend(MQTTAsync_successData* response) {
      TRC_DEBUG(
        CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
        "Message sent successfully: " << NAME_PAR(token, (response ? response->token : 0))
      );
    }

    //------------------------
    static void s_onSendFailure(void* context, MQTTAsync_failureData* response) {
      ((MqttMessagingImpl*)context)->onSendFailure(response);
    }
    void onSendFailure(MQTTAsync_failureData* response) {
      TRC_WARNING(
        CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
        "Message sent failure: " << PAR(response->code)
      );
    }

    //------------------------
    static void s_connlost(void *context, char *cause) {
      ((MqttMessagingImpl*)context)->connlost(cause);
    }
    void connlost(char *cause) {
      {
        std::unique_lock<std::mutex> lck(m_connectionMutex);
        m_connected = false;
      }

      TRC_WARNING(
        CONNECTION(m_mqttBrokerAddr, m_mqttClientId) <<
        "Connection lost: " << NAME_PAR(cause, (cause ? cause : "nullptr"))
      );
    }

    //------------------------
    static void s_onDisconnect(void* context, MQTTAsync_successData* response) {
      ((MqttMessagingImpl*)context)->onDisconnect(response);
    }
    void onDisconnect(MQTTAsync_successData* response) {
      TRC_DEBUG(NAME_PAR(token, (response ? response->token : 0)));
      m_disconnect_promise.set_value(true);
    }
  };

  //////////////////

  MqttMessaging::MqttMessaging()
  {
    TRC_FUNCTION_ENTER("");
    m_impl = shape_new MqttMessagingImpl();
    TRC_FUNCTION_LEAVE("")
  }

  MqttMessaging::~MqttMessaging()
  {
    TRC_FUNCTION_ENTER("");
    delete m_impl;
    TRC_FUNCTION_LEAVE("")
  }

  void MqttMessaging::registerMessageHandler(MessageHandlerFunc hndl)
  {
    TRC_FUNCTION_ENTER("");
    m_impl->registerMessageHandler(hndl);
    TRC_FUNCTION_LEAVE("")
  }

  void MqttMessaging::unregisterMessageHandler()
  {
    TRC_FUNCTION_ENTER("");
    m_impl->unregisterMessageHandler();
    TRC_FUNCTION_LEAVE("")
  }

  void MqttMessaging::sendMessage(const std::string& messagingId, const std::basic_string<uint8_t> & msg)
  {
    TRC_FUNCTION_ENTER(PAR(messagingId));
    m_impl->sendMessage(msg);
    TRC_FUNCTION_LEAVE("")
  }

  const std::string &  MqttMessaging::getName() const
  {
    return m_impl->getName();
  }

  bool MqttMessaging::acceptAsyncMsg() const
  {
    return m_impl->acceptAsyncMsg();
  }

  void MqttMessaging::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "MqttMessaging instance activate" << std::endl <<
      "******************************"
    );

    modify(props);

    m_impl->start();

    TRC_FUNCTION_LEAVE("")
  }

  void MqttMessaging::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "MqttMessaging instance deactivate" << std::endl <<
      "******************************"
    );

    m_impl->stop();

    TRC_FUNCTION_LEAVE("")
  }

  void MqttMessaging::modify(const shape::Properties *props)
  {
    m_impl->update(props);
  }

  void MqttMessaging::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void MqttMessaging::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
