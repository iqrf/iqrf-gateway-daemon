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

#define IIqrfChannelService_EXPORTS

#include "GTestStaticRunner.h"
#include "TestServiceMode.h"
#include "Trace.h"
#include "DatabaseUtils.h"
#include "DateTimeUtils.h"
#include "TestUtils.h"
#include "api_token_repo.hpp"

#include "gtest/gtest.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

#include "iqrf__TestServiceMode.hxx"


TRC_INIT_MNAME(iqrf::TestServiceMode)

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;
using namespace rapidjson;

namespace iqrf {

  class Imp {
  private:
    Imp()
    {
    }

  public:
    shape::ILaunchService* m_iLaunchService = nullptr;
    iqrf::ITestSimulationIqrfChannel* m_iTestSimulationIqrfChannel = nullptr;
    iqrf::ITestSimulationMessaging* m_iTestSimulationMessaging = nullptr;

    shape::GTestStaticRunner m_gtest;

    static Imp& get()
    {
      static Imp imp;
      return imp;
    }

    ~Imp()
    {
    }

    void activate(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestServiceMode instance activate" << std::endl <<
        "******************************"
      );

      m_gtest.runAllTests(m_iLaunchService);

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestServiceMode instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
    {
      m_iTestSimulationIqrfChannel = iface;
    }

    void detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
    {
      if (m_iTestSimulationIqrfChannel == iface) {
        m_iTestSimulationIqrfChannel = nullptr;
      }
    }

    void attachInterface(iqrf::ITestSimulationMessaging* iface)
    {
      m_iTestSimulationMessaging = iface;
    }

    void detachInterface(iqrf::ITestSimulationMessaging* iface)
    {
      if (m_iTestSimulationMessaging == iface) {
        m_iTestSimulationMessaging = nullptr;
      }
    }

    void attachInterface(shape::ILaunchService* iface)
    {
      m_iLaunchService = iface;
    }

    void detachInterface(shape::ILaunchService* iface) {
      if (m_iLaunchService == iface) {
        m_iLaunchService = nullptr;
      }
    }
  };

  ////////////////////////////////////
  TestServiceMode::TestServiceMode()
  {
  }

  TestServiceMode::~TestServiceMode()
  {
  }

  void TestServiceMode::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestServiceMode::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestServiceMode::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void TestServiceMode::attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestServiceMode::detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestServiceMode::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestServiceMode::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestServiceMode::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestServiceMode::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestServiceMode::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestServiceMode::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  //////////////////////////////////////////////////////////////////
  class ServiceModeTest : public ::testing::Test {
  protected:
    net::io_context io_context;
    tcp::resolver resolver{io_context};
    std::shared_ptr<SQLite::Database> db;

    db::models::ApiToken noservice{
      1,
      "noservice",
      "Km94ufh80JgJAW5ryvTmXw==",
      "OZjVJ/KYRfJ9FALd3VCY7+z3zsyDDa6kGVvP1K0unL0=",
      0,
      0,
      false,
      false
    };
    db::models::ApiToken service{
      2,
      "service",
      "S1pu3+64CglEyzR+s7+Evw==",
      "zsRbb+1AYkZi+DooboxUrvejyay4YJ7jJbny9xN8xH8=",
      0,
      0,
      false,
      true
    };
    const std::string no_service_string = "iqrfgd2;1;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio=";
    const std::string service_string = "iqrfgd2;2;E75vLfBqxutkVuHl16nqLHPplttSly2nmZ82YRrvd0E=";

    void SetUp() override {
      ASSERT_NE(nullptr, &Imp::get().m_iLaunchService);
      auto path = Imp::get().m_iLaunchService->getConfigurationDir() + "/DB/IqrfAuthDb.db";
      db = create_database_connetion(path, false, 500, true);
      SQLite::Statement stmt(*db, "PRAGMA journal_mode;");
      ASSERT_TRUE(stmt.executeStep());
      ASSERT_STREQ("wal", stmt.getColumn(0).getText());
      ASSERT_TRUE(db->tableExists("api_tokens"));
      auto timestamp = DateTimeUtils::get_current_timestamp();
      auto expiration = timestamp + 31536000;
      removeToken(noservice.getId());
      removeToken(service.getId());
      insertToken(noservice, timestamp, expiration);
      insertToken(service, timestamp, expiration);
    }

    void insertToken(db::models::ApiToken& token, int64_t created_at, int64_t expiration) {
      SQLite::Statement stmt(*db,
        R"(
        INSERT OR IGNORE INTO api_tokens (id, owner, salt, hash, createdAt, expiresAt, revoked, service)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
        )"
      );
      stmt.bind(1, token.getId());
      stmt.bind(2, token.getOwner());
      stmt.bind(3, token.getSalt());
      stmt.bind(4, token.getHash());
      stmt.bind(5, created_at);
      stmt.bind(6, expiration);
      stmt.bind(7, token.isRevoked());
      stmt.bind(8, token.canUseServiceMode());
      try {
        stmt.exec();
      } catch (const std::exception &e) {
        FAIL() << "Failed to insert API key to database." << e.what();
      }
    }

    void removeToken(uint32_t id) {
      SQLite::Statement stmt(*db,
        R"(
        DELETE FROM api_tokens
        WHERE id = ?
        )"
      );
      stmt.bind(1, id);
      try {
        stmt.exec();
      } catch (const std::exception &e) {
        FAIL() << "Failed to remove token." << e.what();
      }
    }

    websocket::stream<tcp::socket> wsConnectAuth(const std::string& token) {
      websocket::stream<tcp::socket> ws{io_context};
      beast::error_code ec;
      auto const resolved = resolver.resolve("localhost", "1338", ec);
      if (ec) {
        throw std::runtime_error("Failed to resolve host.");
      }
      net::connect(ws.next_layer(), resolved, ec);
      if (ec) {
        throw std::runtime_error("Failed to connect.");
      }
      ws.handshake("localhost", "/", ec);
      if (ec) {
        throw std::runtime_error("Failed to perform handshake host.");
      }
      // do successful auth
      json doc({
        {"type", "auth"},
        {"token", token}
      });
      ws.write(net::buffer(doc.dump()), ec);
      if (ec) {
        throw std::runtime_error("Failed to write authentication message.");
      }
      // read auth success
      beast::flat_buffer buffer;
      ws.read(buffer, ec);
      if (ec) {
        throw std::runtime_error("Failed to retrieve authentication success message.");
      }
      buffer.consume(buffer.size());
      return ws;
    }

    static constexpr std::string_view MODE_SERVICE = R"({
      "mType": "mngDaemon_Mode",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "req": {
          "operMode": "service"
        },
        "returnVerbose": true
      }
    })";

    static constexpr std::string_view MODE_OPERATIONAL = R"({
      "mType": "mngDaemon_Mode",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "req": {
          "operMode": "operational"
        },
        "returnVerbose": true
      }
    })";

    static constexpr std::string_view MODE_FORWARDING = R"({
      "mType": "mngDaemon_Mode",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "req": {
          "operMode": "forwarding"
        },
        "returnVerbose": true
      }
    })";

    static constexpr std::string_view ACTIVATE = R"({
      "mType": "mngService_Activate",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "returnVerbose": true
      }
    })";
    static constexpr std::string_view DEACTIVATE = R"({
      "mType": "mngService_Deactivate",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "returnVerbose": true
      }
    })";
    static constexpr std::string_view GWIDENT = R"({
      "mType": "mngService_GwIdentification",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "returnVerbose": true
      }
    })";
    static constexpr std::string_view TRINFO = R"({
      "mType": "mngService_TrInfo",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "returnVerbose": true
      }
    })";
    static constexpr std::string_view TRWRITE = R"({
      "mType": "mngService_TrWrite",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "req": {
          "packet": [0,0,6,3,255,255]
        },
        "returnVerbose": true
      }
    })";
    static constexpr std::string_view RAW = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "test_raw",
        "req": {
          "rData": "00.00.06.03.ff.ff"
        },
        "returnVerbose": true
      }
    })";
  };

  TEST_F(ServiceModeTest, no_websocket_gwident) {
    // send gw identification
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(std::string(ServiceModeTest::GWIDENT));
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    // read gw identification failed
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_GwIdentification", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(2, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service API is only available for WebSocket connections.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, no_websocket_trinfo) {
    // send tr information
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(std::string(ServiceModeTest::TRINFO));
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    // read tr information failed
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrInfo", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(2, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service API is only available for WebSocket connections.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, no_websocket_activate) {
    // send activate
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(std::string(ServiceModeTest::ACTIVATE));
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    // read activate failed
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(2, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service API is only available for WebSocket connections.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, no_websocket_deactivate) {
    // send deactivate
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(std::string(ServiceModeTest::DEACTIVATE));
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    // read deactivate failed
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Deactivate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(2, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service API is only available for WebSocket connections.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, no_websocket_trwrite) {
    // send tr write
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(std::string(ServiceModeTest::TRWRITE));
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    // read tr write failed
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrWrite", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(2, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service API is only available for WebSocket connections.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, no_service_gwident) {
    auto ws = wsConnectAuth(no_service_string);
    beast::error_code ec;
    // send gw identification
    ws.write(net::buffer(ServiceModeTest::GWIDENT), ec);
    ASSERT_FALSE(ec);
    // read gw identificaation failed
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_GwIdentification", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(3, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("WebSocket API token does not have sufficient permissions to use Service API.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, no_service_trinfo) {
    auto ws = wsConnectAuth(no_service_string);
    beast::error_code ec;
    // send tr information
    ws.write(net::buffer(ServiceModeTest::TRINFO), ec);
    ASSERT_FALSE(ec);
    // read tr information failed
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrInfo", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(3, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("WebSocket API token does not have sufficient permissions to use Service API.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, no_service_activate) {
    auto ws = wsConnectAuth(no_service_string);
    beast::error_code ec;
    // send activate
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate failed
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(3, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("WebSocket API token does not have sufficient permissions to use Service API.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, no_service_deactivate) {
    auto ws = wsConnectAuth(no_service_string);
    beast::error_code ec;
    // send deactivate
    ws.write(net::buffer(ServiceModeTest::DEACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read deactivate failed
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Deactivate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(3, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("WebSocket API token does not have sufficient permissions to use Service API.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, no_service_trwrite) {
    auto ws = wsConnectAuth(no_service_string);
    beast::error_code ec;
    // send tr write
    ws.write(net::buffer(ServiceModeTest::TRWRITE), ec);
    ASSERT_FALSE(ec);
    // read tr write failed
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrWrite", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(3, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("WebSocket API token does not have sufficient permissions to use Service API.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_gwident) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send gw identification
    ws.write(net::buffer(ServiceModeTest::GWIDENT), ec);
    ASSERT_FALSE(ec);
    // read gw identification success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_GwIdentification", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("iqrf-gateway-daemon", Pointer("/data/rsp/name").Get(doc)->GetString());
    EXPECT_STREQ("testhost", Pointer("/data/rsp/hostname").Get(doc)->GetString());
    EXPECT_EQ(2232, Pointer("/data/rsp/osBuild").Get(doc)->GetInt());
    EXPECT_EQ(56, Pointer("/data/rsp/osVersion").Get(doc)->GetInt());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_trinfo) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send tr information
    ws.write(net::buffer(ServiceModeTest::TRINFO), ec);
    ASSERT_FALSE(ec);
    // read tr information success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrInfo", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(2164281994, Pointer("/data/rsp/mid").Get(doc)->GetUint64());
    EXPECT_EQ(2232, Pointer("/data/rsp/osBuild").Get(doc)->GetInt());
    EXPECT_EQ(56, Pointer("/data/rsp/osVersion").Get(doc)->GetInt());
    EXPECT_EQ(36, Pointer("/data/rsp/trMcuType").Get(doc)->GetInt());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_activate_deactivate) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send deactivate
    ws.write(net::buffer(ServiceModeTest::DEACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read deactivate success
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Deactivate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_activate_while_legacy_mode_active) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send legacy service mode
    ws.write(net::buffer(ServiceModeTest::MODE_SERVICE), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("service", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send activate
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(7, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Legacy service mode is active.", Pointer("/data/statusStr").Get(doc)->GetString());
    // send legacy operational mode
    ws.write(net::buffer(ServiceModeTest::MODE_OPERATIONAL), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("operational", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_activate_while_new_mode_active) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send activate again
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate failure
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(4, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service mode has already been activated.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_activate_after_another_service_client_disconnects) {
    auto ws1 = wsConnectAuth(service_string);
    auto ws2 = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate from client 1
    ws1.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws1.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send activate from client 2
    ws1.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate failed
    ws1.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(4, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service mode has already been activated.", Pointer("/data/statusStr").Get(doc)->GetString());
    // disconnect client 1, triggering automatic service mode deactivation as a result of owner disconnecting
    ws1.close(boost::beast::websocket::close_code::normal, ec);
    ASSERT_FALSE(ec);
    // send activate from client 2 again
    ws2.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    ws2.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_activate_then_legacy_service) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send legacy service mode
    ws.write(net::buffer(ServiceModeTest::MODE_SERVICE), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("service", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_STREQ("WebSocket service mode is active.", Pointer("/data/errorStr").Get(doc)->GetString());
    EXPECT_EQ(-1, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("err", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_activate_then_legacy_operational) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send legacy service mode
    ws.write(net::buffer(ServiceModeTest::MODE_OPERATIONAL), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("service", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_STREQ("WebSocket service mode is active.", Pointer("/data/errorStr").Get(doc)->GetString());
    EXPECT_EQ(-1, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("err", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_activate_then_legacy_forwarding) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send legacy service mode
    ws.write(net::buffer(ServiceModeTest::MODE_FORWARDING), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("service", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_STREQ("WebSocket service mode is active.", Pointer("/data/errorStr").Get(doc)->GetString());
    EXPECT_EQ(-1, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("err", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_activate_legacy_execute_nonwhitelisted_message) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send legacy service mode
    ws.write(net::buffer(ServiceModeTest::MODE_SERVICE), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("service", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send raw message
    ws.write(net::buffer(ServiceModeTest::RAW), ec);
    ASSERT_FALSE(ec);
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("messageError", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_raw", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("Message type iqrfRaw not supported in service mode.", Pointer("/data/rsp/error").Get(doc)->GetString());
    EXPECT_EQ(10, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service mode is active.", Pointer("/data/statusStr").Get(doc)->GetString());
    // send legacy operational mode
    ws.write(net::buffer(ServiceModeTest::MODE_OPERATIONAL), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("operational", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_activate_execute_nonwhitelisted_message) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send raw message
    ws.write(net::buffer(ServiceModeTest::RAW), ec);
    ASSERT_FALSE(ec);
    // read raw message error
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("messageError", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_raw", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("Message type iqrfRaw not supported in service mode.", Pointer("/data/rsp/error").Get(doc)->GetString());
    EXPECT_EQ(10, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service mode is active.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_deactivate_while_inactive) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send deactivate
    ws.write(net::buffer(ServiceModeTest::DEACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read deactivate fail
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Deactivate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(5, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service mode has already been deactivated.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_deactive_while_legacy_service_mode) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send legacy service mode
    ws.write(net::buffer(ServiceModeTest::MODE_SERVICE), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("service", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send deactivate
    ws.write(net::buffer(ServiceModeTest::DEACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read deactivate failed
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Deactivate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(7, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Legacy service mode is active.", Pointer("/data/statusStr").Get(doc)->GetString());
    // send legacy operational mode
    ws.write(net::buffer(ServiceModeTest::MODE_OPERATIONAL), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("operational", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_deactivate_not_service_session_owner) {
    auto ws1 = wsConnectAuth(service_string);
    auto ws2 = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate from client 1
    ws1.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws1.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send deactivate from client 2
    ws2.write(net::buffer(ServiceModeTest::DEACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read deactivate failure
    ws2.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Deactivate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(8, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Another session manages service mode.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_trwrite_not_active) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send tr write
    ws.write(net::buffer(ServiceModeTest::TRWRITE), ec);
    ASSERT_FALSE(ec);
    // read tr write failed
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrWrite", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(6, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service mode is not active.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_trwrite_legacy_service_mode) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send legacy service mode
    ws.write(net::buffer(ServiceModeTest::MODE_SERVICE), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("service", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send tr write
    ws.write(net::buffer(ServiceModeTest::TRWRITE), ec);
    ASSERT_FALSE(ec);
    // read tr write failed
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrWrite", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(7, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Legacy service mode is active.", Pointer("/data/statusStr").Get(doc)->GetString());
    // send legacy operational mode
    ws.write(net::buffer(ServiceModeTest::MODE_OPERATIONAL), ec);
    ASSERT_FALSE(ec);
    // read legacy service success
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("operational", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_trwrite_not_service_session_owner) {
    auto ws1 = wsConnectAuth(service_string);
    auto ws2 = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate from client 1
    ws1.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws1.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send tr write from client 2
    ws2.write(net::buffer(ServiceModeTest::TRWRITE), ec);
    ASSERT_FALSE(ec);
    // read tr write failure
    ws2.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrWrite", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(8, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Another session manages service mode.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeTest, service_trwrite_success) {
    auto ws = wsConnectAuth(service_string);
    beast::error_code ec;
    // send activate
    ws.write(net::buffer(ServiceModeTest::ACTIVATE), ec);
    ASSERT_FALSE(ec);
    // read activate success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_Activate", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // send tr write
    ws.write(net::buffer(ServiceModeTest::TRWRITE), ec);
    ASSERT_FALSE(ec);
    // enque DPA response
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage("00.00.06.83.00.00.00.44", 1000);
    // read activate success
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrWrite", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // read async response
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    response = beast::buffers_to_string(buffer.data());
    buffer.consume(buffer.size());
    doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());
    // check response
    EXPECT_STREQ("mngService_TrData", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("async", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Ok", Pointer("/data/statusStr").Get(doc)->GetString());
    std::vector<int> expected = {0x00, 0x00, 0x06, 0x83, 0x00, 0x00, 0x00, 0x44};
    const auto& val = Pointer("/data/rsp/packet").Get(doc)->GetArray();
    ASSERT_EQ(expected.size(), val.Size());
    for (rapidjson::SizeType i{}; i < val.Size(); ++i) {
      ASSERT_TRUE(val[i].IsInt());
      EXPECT_EQ(val[i].GetInt(), expected[i]);
    }
  }

}
