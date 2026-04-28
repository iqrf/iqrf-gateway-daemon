/**
 * Copyright 2015-2026 IQRF Tech s.r.o.
 * Copyright 2019-2026 MICRORISC s.r.o.
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

#include "DatetimeParser.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/websocket/error.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/websocket/stream_base.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#define IIqrfChannelService_EXPORTS

#include "TestWebsocketMessagingAuth.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "CryptoUtils.h"
#include "DatabaseUtils.h"
#include "api_token_repo.hpp"

#include "gtest/gtest.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include "iqrf__TestWebsocketMessagingAuth.hxx"

TRC_INIT_MNAME(iqrf::TestWebsocketMessagingAuth)

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

namespace iqrf {

  class Imp {
  private:
    Imp()
    {
    }

  public:
    shape::ILaunchService* m_iLaunchService = nullptr;
    iqrf::ITestSimulationIqrfChannel* m_iTestSimulationIqrfChannel = nullptr;

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
        "TestWebsocketMessagingAuth instance activate" << std::endl <<
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
        "TestWebsocketMessagingAuth instance deactivate" << std::endl <<
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

    void attachInterface(shape::ILaunchService* iface)
    {
      m_iLaunchService = iface;
    }

    void detachInterface(shape::ILaunchService* iface)
    {
      if (m_iLaunchService == iface) {
        m_iLaunchService = nullptr;
      }
    }

  };

  ////////////////////////////////////
  TestWebsocketMessagingAuth::TestWebsocketMessagingAuth()
  {
  }

  TestWebsocketMessagingAuth::~TestWebsocketMessagingAuth()
  {
  }

  void TestWebsocketMessagingAuth::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestWebsocketMessagingAuth::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestWebsocketMessagingAuth::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void TestWebsocketMessagingAuth::attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestWebsocketMessagingAuth::detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestWebsocketMessagingAuth::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestWebsocketMessagingAuth::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestWebsocketMessagingAuth::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestWebsocketMessagingAuth::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class WebsocketMessagingAuthTest : public ::testing::Test {
  protected:
    net::io_context io_context;
    tcp::resolver resolver{io_context};
    std::shared_ptr<SQLite::Database> db;
    db::models::ApiToken valid_token{
      1,
      "valid_test",
      "q3qaIzd74DZFnYkhCyzwFI+OPgA+o/bs0461sPzo6bA=",
      std::chrono::system_clock::time_point::min(),
      std::chrono::system_clock::time_point::min(),
      ApiToken::Status::Valid,
      false,
      std::nullopt,
    };
    db::models::ApiToken revoked_token{
      2,
      "revoked_test",
      "YKyPGf9jfXN0Lj71LMJm5U3ZaSOXgYuTeNtqNu6fkE0=",
      std::chrono::system_clock::time_point::min(),
      std::chrono::system_clock::time_point::min(),
      ApiToken::Status::Revoked,
      false,
      std::nullopt
    };
    db::models::ApiToken expired_token{
      3,
      "expired_test",
      "L+3UfZb24CPMrOpyUIxW/1m+HnAQV2wr3PpDvxOx1c8=",
      std::chrono::system_clock::time_point::min(),
      std::chrono::system_clock::time_point::min(),
      ApiToken::Status::Expired,
      false,
      std::nullopt
    };
    db::models::ApiToken revoked_later_token{
      4,
      "revoked_later_token",
      "m6qZai36k7ZSJjZ32GWTAaO/xp0NzyLnOw/gc9NJsUQ=",
      std::chrono::system_clock::time_point::min(),
      std::chrono::system_clock::time_point::min(),
      ApiToken::Status::Valid,
      true,
      std::nullopt
    };
    const std::string valid_token_string = "iqrfgd2;1;25SI8PrptHDbgDGCtfJHbaBCaqyYIMxuQpXpsZrvoU4=";
    const std::string revoked_token_string = "iqrfgd2;2;4sT5KP+8U4X0b61LdOX1FL9r8umEUimzzK+lAcoxDOc=";
    const std::string expired_token_string = "iqrfgd2;3;MuekuECy4EJOI2REYQ7zC0hiQ6WiUNU7Jj86qTjCbyE=";
    const std::string revoked_later_token_string = "iqrfgd2;4;dFYIlOolYq3bDmwxsMLy03HfJzLOmoeTumJfXnsBTJA=";

    void insertToken(
      db::models::ApiToken& token,
      std::chrono::system_clock::time_point created_at,
      std::chrono::system_clock::time_point expires_at
    ) {
      SQLite::Statement stmt(*db,
        R"(
        INSERT OR IGNORE INTO api_tokens (id, owner, hash, createdAt, expiresAt, status, service)
        VALUES (?, ?, ?, ?, ?, ?, ?);
        )"
      );

      stmt.bind(1, token.getId());
      stmt.bind(2, token.getOwner());
      stmt.bind(3, token.getHash());
      stmt.bind(4, DatetimeParser::toISO8601(created_at));
      stmt.bind(5, DatetimeParser::toISO8601(expires_at));
      stmt.bind(6, static_cast<int>(token.getStatus()));
      stmt.bind(7, token.canUseServiceMode());
      try {
        stmt.exec();
      } catch (const std::exception &e) {
        FAIL() << "Failed to insert API key to database." << e.what();
      }
    }

    void revokeToken(uint32_t id) {
      db::repos::ApiTokenRepository repo(db);
      try {
        repo.revoke(id, std::chrono::system_clock::now());
      } catch (const std::exception &e) {
        FAIL() << "Failed to revoke API key." << e.what();
      }
    }

    void removeToken(uint32_t id) {
      db::repos::ApiTokenRepository repo(db);
      try {
        repo.remove(id);
      } catch (const std::exception &e) {
        FAIL() << "Failed to remove token." << e.what();
      }
    }

    std::string generateRandomPrintable(std::size_t len) {
      static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "!@#$%^&*()-_=+[]{};:,.<>/?";

      static thread_local std::mt19937 generator{std::random_device{}()};
      static thread_local std::uniform_int_distribution<std::size_t> distribution(0, sizeof(charset) - 2);

      std::string data;
      data.reserve(len);

      for (std::size_t i{}; i < len; ++i) {
        data.push_back(charset[distribution(generator)]);
      }

      return data;
    }
  };

  class WebsocketMessagingPlainAuthTest : public WebsocketMessagingAuthTest {
  protected:
    websocket::stream<tcp::socket> ws{io_context};

    void SetUp() override {
      ASSERT_NE(nullptr, &Imp::get().m_iLaunchService);
      beast::error_code ec;
      auto const resolved = resolver.resolve("localhost", "1338", ec);
      ASSERT_FALSE(ec);
      net::connect(ws.next_layer(), resolved, ec);
      ASSERT_FALSE(ec);
      ws.handshake("localhost", "/", ec);
      ASSERT_FALSE(ec);
      ASSERT_TRUE(ws.is_open());
      auto path = Imp::get().m_iLaunchService->getConfigurationDir() + "/DB/IqrfAuthDb.db";
      db = create_database_connetion(path, false, 500, true);
      SQLite::Statement stmt(*db, "PRAGMA journal_mode;");
      ASSERT_TRUE(stmt.executeStep());
      ASSERT_STREQ("wal", stmt.getColumn(0).getText());
      ASSERT_TRUE(db->tableExists("api_tokens"));
    }
  };

  class WebsocketMessagingTlsAuthTest : public WebsocketMessagingAuthTest {
  protected:
    ssl::context ctx{ssl::context::tlsv12_client};
    std::optional<websocket::stream<ssl::stream<tcp::socket>>> ws;

    void SetUp() override {
      ASSERT_NE(nullptr, &Imp::get().m_iLaunchService);
      auto cert_path = Imp::get().m_iLaunchService->getConfigurationDir() + "/certs/cert.pem";
      ctx.load_verify_file(cert_path);
      ctx.set_verify_mode(ssl::verify_peer);
      ws.emplace(io_context, ctx);
      beast::error_code ec;
      auto const resolved = resolver.resolve("localhost", "8338", ec);
      ASSERT_FALSE(ec);
      net::connect(beast::get_lowest_layer(*ws), resolved, ec);
      ASSERT_FALSE(ec);
      ws->next_layer().set_verify_callback(ssl::host_name_verification("localhost"), ec);
      ASSERT_FALSE(ec);
      ws->next_layer().handshake(ssl::stream_base::client);
      ws->set_option(
        websocket::stream_base::decorator(
          [](websocket::request_type& req) {
            req.set(
              http::field::user_agent,
              std::string(BOOST_BEAST_VERSION_STRING) + " ws-auth-tls-test"
            );
          }
        )
      );
      ws->handshake("localhost", "/", ec);
      ASSERT_FALSE(ec);
      ASSERT_TRUE(ws->is_open());
      auto path = Imp::get().m_iLaunchService->getConfigurationDir() + "/DB/IqrfAuthDb.db";
      db = create_database_connetion(path, false, 500, true);
      SQLite::Statement stmt(*db, "PRAGMA journal_mode;");
      ASSERT_TRUE(stmt.executeStep());
      ASSERT_STREQ("wal", stmt.getColumn(0).getText());
      ASSERT_TRUE(db->tableExists("api_tokens"));
    }

  };

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messsaging_invalid_message_type) {
    beast::error_code ec;

    ws.write(net::buffer("invalid"));
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":3,"error":"Authentication failed","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_not_auth_messsage) {
    beast::error_code ec;

    json doc({
      {"type", "invalid"}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":1,"error":"Unauthenticated","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_timeout) {
    beast::error_code ec;

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":2,"error":"Authentication timeout","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_invalid_token_format) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "token"}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_unsupported_api_token) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqaros;1;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_nonnumeric_api_token_id) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqrfgd2;1a;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_invalid_secret_len) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqrfgd2;1a;zDrcvQaXWopzJ+o="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_invalid_secret_characters) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqrfgd2;1;zDrcvQaXWopzJ-DbfkpGq3Tn00wkt3n*fExj8iUsYio="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_nonexistent_token) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqrfgd2;15;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_revoked_token) {
    beast::error_code ec;

    auto timestamp = std::chrono::system_clock::now();
    insertToken(revoked_token, timestamp, timestamp + std::chrono::hours(24 * 365));

    json doc({
      {"type", "auth"},
      {"token", revoked_token_string}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":6,"error":"Revoked token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_expired_token) {
    beast::error_code ec;

    auto timestamp = std::chrono::system_clock::now();
    insertToken(expired_token, timestamp - std::chrono::hours(24 * 30), timestamp - std::chrono::hours(1));

    json doc({
      {"type", "auth"},
      {"token", expired_token_string}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":5,"error":"Expired token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_success) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // attempt to communicate after websocket auth successful
    std::string test_request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "auth_success_test",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
        }
      }
    })";

    ws.write(net::buffer(test_request), ec);
    ASSERT_FALSE(ec);

    // simulate response from network
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage("00.00.06.83.00.00.00.44", 100);
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    expected = R"({"mType":"iqrfRaw","data":{"msgId":"auth_success_test","rsp":{"rData":"00.00.06.83.00.00.00.44"},"status":0,"insId":"iqrfgd2-default"}})";
    EXPECT_EQ(expected, received);
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_after_auth_success) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // send another auth message
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read unexpected auth message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    expected = R"({"mType":"messageError","data":{"msgId":"auth","rsp":{"error":"Received a duplicate or unexpected auth message."},"status":9,"statusStr":"Unexpected auth message.","insId":"iqrfgd2-default"}})";
    EXPECT_EQ(expected, received);
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_success_expired_before_client_request) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::seconds(5);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // wait until token expires
    std::this_thread::sleep_for(std::chrono::seconds(6));
    // attempt to communicate after auth successful
    std::string test_request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "auth_success_test",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
        }
      }
    })";

    ws.write(net::buffer(test_request), ec);
    ASSERT_FALSE(ec);
    // Read expired token message message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":5,"error":"Expired token","type":"auth_error"})");
    // clear buffer
    buffer.consume(buffer.size());
    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
    removeToken(valid_token.getId());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_success_revoked_after) {
    beast::error_code ec;

    removeToken(revoked_later_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(revoked_later_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", revoked_later_token_string}
    });
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":true,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // attempt to communicate after websocket auth successful
    std::string test_request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "auth_success_test",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
        }
      }
    })";

    ws.write(net::buffer(test_request), ec);
    ASSERT_FALSE(ec);

    // simulate response from network
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage("00.00.06.83.00.00.00.44", 100);
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    expected = R"({"mType":"iqrfRaw","data":{"msgId":"auth_success_test","rsp":{"rData":"00.00.06.83.00.00.00.44"},"status":0,"insId":"iqrfgd2-default"}})";
    EXPECT_EQ(expected, received);
    // clear buffer
    buffer.consume(buffer.size());

    // revoke token
    revokeToken(revoked_later_token.getId());
    // Read revoked token message message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":6,"error":"Revoked token","type":"auth_error"})");
    // clear buffer
    buffer.consume(buffer.size());
    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_auth_success_deleted_after) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
      // read auth success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    buffer.consume(buffer.size());
    // attempt to communicate after websocket auth successful
    std::string test_request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "auth_success_test",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
        }
      }
    })";

    ws.write(net::buffer(test_request), ec);
    ASSERT_FALSE(ec);

    // simulate response from network
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage("00.00.06.83.00.00.00.44", 100);
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    auto received = beast::buffers_to_string(buffer.data());
    std::string expected = R"({"mType":"iqrfRaw","data":{"msgId":"auth_success_test","rsp":{"rData":"00.00.06.83.00.00.00.44"},"status":0,"insId":"iqrfgd2-default"}})";
    EXPECT_EQ(expected, received);
    // clear buffer
    buffer.consume(buffer.size());

    // revoke token
    removeToken(valid_token.getId());
    // Read revoked token message message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");
    // clear buffer
    buffer.consume(buffer.size());
    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::internal_error);
    EXPECT_FALSE(ws.is_open());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_client_payload_too_large) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // send message exceeding accepted received message size limit
    ws.write(
      net::buffer(generateRandomPrintable(100000)),
      ec
    );
    ASSERT_FALSE(ec);
    // receive close frame and check close reason - too big
    ws.read(buffer, ec);
    EXPECT_TRUE(
      ec == websocket::error::closed ||
      ec == boost::asio::error::eof ||
      ec == boost::asio::error::connection_reset ||
      ec == boost::asio::error::broken_pipe ||
      ec == boost::asio::error::not_connected
    );
    EXPECT_EQ(ws.reason().code, websocket::close_code::too_big);
    EXPECT_FALSE(ws.is_open());
    buffer.consume(buffer.size());
  }

  TEST_F(WebsocketMessagingPlainAuthTest, test_websocket_messaging_client_capacity) {
    beast::error_code ec;
    beast::flat_buffer buffer;

    ws.close(beast::websocket::close_code::normal, ec);
    ASSERT_FALSE(ec);

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);

    // create streams within capacity
    auto const resolved = resolver.resolve("localhost", "1338", ec);
    ASSERT_FALSE(ec);

    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });

    std::vector<std::shared_ptr<websocket::stream<tcp::socket>>> streams = {};
    for (std::size_t i{}; i < 50; ++i) {
      auto stream = std::make_shared<websocket::stream<tcp::socket>>(io_context);
      net::connect(stream->next_layer(), resolved, ec);
      ASSERT_FALSE(ec);
      stream->handshake("localhost", "/", ec);
      ASSERT_FALSE(ec);
      ASSERT_TRUE(stream->is_open());

      // do successful auth
      stream->write(net::buffer(doc.dump()), ec);
      ASSERT_FALSE(ec);
      // read auth success
      stream->read(buffer, ec);
      ASSERT_FALSE(ec);
      std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
      std::string received = beast::buffers_to_string(buffer.data());
      EXPECT_EQ(expected, received);
      buffer.consume(buffer.size());
      // store for later
      streams.push_back(stream);
    }

    auto stream = websocket::stream<tcp::socket>{io_context};
    net::connect(stream.next_layer(), resolved, ec);
    ASSERT_FALSE(ec);
    stream.read(buffer, ec);
    ASSERT_TRUE(ec);
    ASSERT_FALSE(stream.is_open());

    for (std::size_t i{}; i < 50; ++i) {
      auto stream = streams.back();
      streams.pop_back();

      stream->close(beast::websocket::close_code::normal, ec);
      ASSERT_FALSE(ec);
    }
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messsaging_invalid_message_type) {
    beast::error_code ec;

    ws->write(net::buffer("invalid"));
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":3,"error":"Authentication failed","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_not_auth_messsage) {
    beast::error_code ec;

    json doc({
      {"type", "invalid"}
    });

    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":1,"error":"Unauthenticated","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_timeout) {
    beast::error_code ec;

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":2,"error":"Authentication timeout","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_invalid_token_format) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "token"}
    });

    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_unsupported_api_token) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqaros;1;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
    });

    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_nonnumeric_api_token_id) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqrfgd2;1a;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
    });

    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_invalid_secret_len) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqrfgd2;1a;zDrcvQaXWopzJ+o="}
    });

    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_invalid_secret_characters) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqrfgd2;1;zDrcvQaXWopzJ-DbfkpGq3Tn00wkt3n*fExj8iUsYio="}
    });

    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_nonexistent_token) {
    beast::error_code ec;

    json doc({
      {"type", "auth"},
      {"token", "iqrfgd2;15;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
    });

    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_revoked_token) {
    beast::error_code ec;

    auto timestamp = std::chrono::system_clock::now();
    insertToken(revoked_token, timestamp, timestamp + std::chrono::hours(24 * 365));

    json doc({
      {"type", "auth"},
      {"token", revoked_token_string}
    });

    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":6,"error":"Revoked token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_expired_token) {
    beast::error_code ec;

    auto timestamp = std::chrono::system_clock::now();
    insertToken(expired_token, timestamp - std::chrono::hours(24 * 30), timestamp - std::chrono::hours(1));

    json doc({
      {"type", "auth"},
      {"token", expired_token_string}
    });

    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":5,"error":"Expired token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_success) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // attempt to communicate after websocket auth successful
    std::string test_request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "auth_success_test",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
        }
      }
    })";

    ws->write(net::buffer(test_request), ec);
    ASSERT_FALSE(ec);

    // simulate response from network
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage("00.00.06.83.00.00.00.44", 100);
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    expected = R"({"mType":"iqrfRaw","data":{"msgId":"auth_success_test","rsp":{"rData":"00.00.06.83.00.00.00.44"},"status":0,"insId":"iqrfgd2-default"}})";
    EXPECT_EQ(expected, received);
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_after_auth_success) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // send another auth message
    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read unexpected auth message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    expected = R"({"mType":"messageError","data":{"msgId":"auth","rsp":{"error":"Received a duplicate or unexpected auth message."},"status":9,"statusStr":"Unexpected auth message.","insId":"iqrfgd2-default"}})";
    EXPECT_EQ(expected, received);
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_success_expired_before_client_request) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::seconds(5);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // wait until token expires
    std::this_thread::sleep_for(std::chrono::seconds(6));
    // attempt to communicate after auth successful
    std::string test_request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "auth_success_test",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
        }
      }
    })";

    ws->write(net::buffer(test_request), ec);
    ASSERT_FALSE(ec);
    // Read expired token message message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":5,"error":"Expired token","type":"auth_error"})");
    // clear buffer
    buffer.consume(buffer.size());
    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
    removeToken(valid_token.getId());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_success_revoked_after) {
    beast::error_code ec;

    removeToken(revoked_later_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(revoked_later_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", revoked_later_token_string}
    });
    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":true,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // attempt to communicate after websocket auth successful
    std::string test_request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "auth_success_test",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
        }
      }
    })";

    ws->write(net::buffer(test_request), ec);
    ASSERT_FALSE(ec);

    // simulate response from network
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage("00.00.06.83.00.00.00.44", 100);
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    expected = R"({"mType":"iqrfRaw","data":{"msgId":"auth_success_test","rsp":{"rData":"00.00.06.83.00.00.00.44"},"status":0,"insId":"iqrfgd2-default"}})";
    EXPECT_EQ(expected, received);
    // clear buffer
    buffer.consume(buffer.size());

    // revoke token
    revokeToken(revoked_later_token.getId());
    // Read revoked token message message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":6,"error":"Revoked token","type":"auth_error"})");
    // clear buffer
    buffer.consume(buffer.size());
    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::policy_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_auth_success_deleted_after) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
      // read auth success
    beast::flat_buffer buffer;
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    buffer.consume(buffer.size());
    // attempt to communicate after websocket auth successful
    std::string test_request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "auth_success_test",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
        }
      }
    })";

    ws->write(net::buffer(test_request), ec);
    ASSERT_FALSE(ec);

    // simulate response from network
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage("00.00.06.83.00.00.00.44", 100);
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    auto received = beast::buffers_to_string(buffer.data());
    std::string expected = R"({"mType":"iqrfRaw","data":{"msgId":"auth_success_test","rsp":{"rData":"00.00.06.83.00.00.00.44"},"status":0,"insId":"iqrfgd2-default"}})";
    EXPECT_EQ(expected, received);
    // clear buffer
    buffer.consume(buffer.size());

    // revoke token
    removeToken(valid_token.getId());
    // Read revoked token message message
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Invalid token","type":"auth_error"})");
    // clear buffer
    buffer.consume(buffer.size());
    // Read close frame and check close reason
    ws->read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws->reason().code, websocket::close_code::internal_error);
    EXPECT_FALSE(ws->is_open());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_client_payload_too_large) {
    beast::error_code ec;

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);
    // do successful auth
    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });
    ws->write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
    // read auth success
    beast::flat_buffer buffer;
    ws->read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(expected, received);
    buffer.consume(buffer.size());
    // send message exceeding accepted received message size limit
    ws->write(
      net::buffer(generateRandomPrintable(100000)),
      ec
    );
    ASSERT_FALSE(ec);
    // receive close frame and check close reason - too big
    ws->read(buffer, ec);
    EXPECT_TRUE(
      ec == websocket::error::closed ||
      ec == boost::asio::error::eof ||
      ec == boost::asio::error::connection_reset ||
      ec == boost::asio::error::broken_pipe ||
      ec == boost::asio::error::not_connected
    );
    EXPECT_EQ(ws->reason().code, websocket::close_code::too_big);
    EXPECT_FALSE(ws->is_open());
    buffer.consume(buffer.size());
  }

  TEST_F(WebsocketMessagingTlsAuthTest, test_websocket_messaging_tls_client_capacity) {
    beast::error_code ec;
    beast::flat_buffer buffer;

    ws->close(beast::websocket::close_code::normal, ec);
    ASSERT_FALSE(ec);
    ws->next_layer().shutdown(ec);
    ASSERT_FALSE(ec);
    beast::get_lowest_layer(*ws).close(ec);
    ASSERT_FALSE(ec);
    ASSERT_FALSE(ws->is_open());

    removeToken(valid_token.getId());
    auto timestamp = std::chrono::system_clock::now();
    auto expiration = timestamp + std::chrono::hours(365 * 24);
    insertToken(valid_token, timestamp, expiration);

    // create streams within capacity
    auto const resolved = resolver.resolve("localhost", "8338", ec);
    ASSERT_FALSE(ec);

    json doc({
      {"type", "auth"},
      {"token", valid_token_string}
    });

    std::vector<std::shared_ptr<websocket::stream<ssl::stream<tcp::socket>>>> streams = {};
    for (std::size_t i{}; i < 50; ++i) {
      auto stream = std::make_shared<websocket::stream<ssl::stream<tcp::socket>>>(io_context, ctx);
      net::connect(beast::get_lowest_layer(*stream), resolved, ec);
      ASSERT_FALSE(ec);
      stream->next_layer().set_verify_callback(ssl::host_name_verification("localhost"), ec);
      ASSERT_FALSE(ec);
      stream->next_layer().handshake(ssl::stream_base::client, ec);
      ASSERT_FALSE(ec);
      stream->handshake("localhost", "/", ec);
      ASSERT_FALSE(ec);
      ASSERT_TRUE(stream->is_open());
      // do successful auth
      stream->write(net::buffer(doc.dump()), ec);
      ASSERT_FALSE(ec);
      // read auth success
      stream->read(buffer, ec);
      ASSERT_FALSE(ec);
      std::string expected = "{\"expiration\":\"" + DatetimeParser::toISO8601(expiration) + "\",\"service\":false,\"type\":\"auth_success\"}";
      std::string received = beast::buffers_to_string(buffer.data());
      EXPECT_EQ(expected, received);
      buffer.consume(buffer.size());
      streams.push_back(stream);
    }

    auto stream = websocket::stream<ssl::stream<tcp::socket>>(io_context, ctx);
    net::connect(beast::get_lowest_layer(stream), resolved, ec);
    ASSERT_FALSE(ec);
    stream.next_layer().set_verify_callback(ssl::host_name_verification("localhost"), ec);
    ASSERT_FALSE(ec);
    stream.next_layer().handshake(ssl::stream_base::client, ec);
    ASSERT_FALSE(ec);
    stream.handshake("localhost", "/", ec);
    ASSERT_TRUE(ec);
    ASSERT_FALSE(stream.is_open());

    for (std::size_t i{}; i < 50; ++i) {
      auto stream = streams.back();
      streams.pop_back();

      stream->close(beast::websocket::close_code::normal, ec);
      ASSERT_FALSE(ec);
      stream->next_layer().shutdown(ec);
      ASSERT_FALSE(ec);
      beast::get_lowest_layer(*stream).close(ec);
      ASSERT_FALSE(ec);
    }
  }

}
