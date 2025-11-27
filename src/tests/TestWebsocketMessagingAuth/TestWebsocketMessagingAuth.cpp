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

#include "TestWebsocketMessagingAuth.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "CryptoUtils.h"
#include "DatabaseUtils.h"
#include "DateTimeUtils.h"
#include "api_token_repo.hpp"

#include "gtest/gtest.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

#include "iqrf__TestWebsocketMessagingAuth.hxx"

TRC_INIT_MNAME(iqrf::TestWebsocketMessagingAuth)

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
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
    iqrf::IApiTokenService* m_tokenService = nullptr;

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

    void attachInterface(iqrf::IApiTokenService* iface)
    {
      m_tokenService = iface;
    }

    void detachInterface(iqrf::IApiTokenService* iface)
    {
      if (m_tokenService == iface) {
        m_tokenService = nullptr;
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

  void TestWebsocketMessagingAuth::attachInterface(iqrf::IApiTokenService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestWebsocketMessagingAuth::detachInterface(iqrf::IApiTokenService* iface)
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
    websocket::stream<tcp::socket> ws{io_context};
    std::shared_ptr<SQLite::Database> db;
    db::models::ApiToken valid_token{
      1,
      "valid_test",
      "Km94ufh80JgJAW5ryvTmXw==",
      "OZjVJ/KYRfJ9FALd3VCY7+z3zsyDDa6kGVvP1K0unL0=",
      0,
      0,
      false,
      false
    };
    db::models::ApiToken revoked_token{
      2,
      "revoked_test",
      "S1pu3+64CglEyzR+s7+Evw==",
      "zsRbb+1AYkZi+DooboxUrvejyay4YJ7jJbny9xN8xH8=",
      0,
      0,
      true,
      false
    };
    db::models::ApiToken expired_token{
      3,
      "expired_test",
      "0QDdubRFO5ex0gDbbzxwHg==",
      "/LPYJ33UI9fZzY2b4+hOGf9lcreR9gmM9cHdPLZkEbg=",
      0,
      0,
      false,
      false
    };
    db::models::ApiToken revoked_later_token{
      4,
      "revoked_later_token",
      "60qpNwDAGdCuuWcutV4gtg==",
      "uohKt+Eg2DYDZfAMYp2ic9bJKDoyibMZqhBisrNH+bI=",
      0,
      0,
      false,
      false,
    };
    const std::string valid_token_string = "iqrfgd2;1;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio=";
    const std::string revoked_token_string = "iqrfgd2;2;E75vLfBqxutkVuHl16nqLHPplttSly2nmZ82YRrvd0E=";
    const std::string expired_token_string = "iqrfgd2;3;xK2LnzYTqVLNNUGYEGWU9VU8LC6xQpuKEQtRav7dcUo=";
    const std::string revoked_later_token_string = "iqrfgd2;4;HzYtNdilRD1XCIX0mIu3Og49buDlvlAvFVmZEowT2HI=";

    void SetUp() override {
      ASSERT_NE(nullptr, &Imp::get().m_iLaunchService);
      beast::error_code ec;
      auto const resolved = resolver.resolve("localhost", "1338", ec);
      ASSERT_FALSE(ec);
      net::connect(ws.next_layer(), resolved, ec);
      ASSERT_FALSE(ec);
      ws.handshake("localhost", "/", ec);
      ASSERT_FALSE(ec);
      auto path = Imp::get().m_iLaunchService->getConfigurationDir() + "/DB/IqrfAuthDb.db";
      db = create_database_connetion(path);
      //SQLite::Statement stmt(*db, "PRAGMA journal_mode;");
      //ASSERT_TRUE(stmt.executeStep());
      //ASSERT_STREQ("wal", stmt.getColumn(0).getText());
      ASSERT_TRUE(db->tableExists("api_tokens"));
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

    void revokeToken(uint32_t id) {
      db::repos::ApiTokenRepository repo(db);
      try {
        repo.revoke(id);
      } catch (const std::exception &e) {
        FAIL() << "Failed to revoke API key." << e.what();
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
  };

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messsaging_invalid_message_type) {
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
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_not_auth_messsage) {
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
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_timeout) {
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
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_invalid_token_format) {
    beast::error_code ec;

    json doc({
      {"auth", "token"}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":5,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_unsupported_api_token) {
    beast::error_code ec;

    json doc({
      {"auth", "iqaros;1;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":5,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_nonnumeric_api_token_id) {
    beast::error_code ec;

    json doc({
      {"auth", "iqrfgd2;1a;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":5,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_invalid_secret_len) {
    beast::error_code ec;

    json doc({
      {"auth", "iqrfgd2;1a;zDrcvQaXWopzJ+o="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":5,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_invalid_secret_characters) {
    beast::error_code ec;

    json doc({
      {"auth", "iqrfgd2;1;zDrcvQaXWopzJ-DbfkpGq3Tn00wkt3n*fExj8iUsYio="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":5,"error":"Invalid token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_nonexistent_token) {
    beast::error_code ec;

    json doc({
      {"auth", "iqrfgd2;15;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Token not found","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_revoked_token) {
    beast::error_code ec;

    auto timestamp = DateTimeUtils::get_current_timestamp();
    insertToken(revoked_token, timestamp, timestamp + 31536000);

    json doc({
      {"auth", revoked_token_string}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":7,"error":"Revoked token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_expired_token) {
    beast::error_code ec;

    auto timestamp = DateTimeUtils::get_current_timestamp();
    insertToken(expired_token, timestamp - 2592000, timestamp - 3600);

    json doc({
      {"auth", expired_token_string}
    });

    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);

    beast::flat_buffer buffer;

    // Read unauthorized error message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    std::string received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":6,"error":"Expired token","type":"auth_error"})");

    // clear buffer
    buffer.consume(buffer.size());

    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_success) {
    beast::error_code ec;

    auto timestamp = DateTimeUtils::get_current_timestamp();
    insertToken(valid_token, timestamp, timestamp + 31536000);
    // do successful auth
    json doc({
      {"auth", valid_token_string}
    });
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
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
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    auto received = beast::buffers_to_string(buffer.data());
    std::string expected =
R"({
    "mType": "iqrfRaw",
    "data": {
        "msgId": "auth_success_test",
        "rsp": {
            "rData": "00.00.06.83.00.00.00.44"
        },
        "status": 0,
        "insId": "iqrfgd2-default"
    }
})";
    EXPECT_EQ(expected, received);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_success_revoked_after) {
    beast::error_code ec;

    removeToken(revoked_later_token.getId());
    auto timestamp = DateTimeUtils::get_current_timestamp();
    insertToken(revoked_later_token, timestamp, timestamp + 31536000);
    // do successful auth
    json doc({
      {"auth", revoked_later_token_string}
    });
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
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
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    auto received = beast::buffers_to_string(buffer.data());
    std::string expected =
R"({
    "mType": "iqrfRaw",
    "data": {
        "msgId": "auth_success_test",
        "rsp": {
            "rData": "00.00.06.83.00.00.00.44"
        },
        "status": 0,
        "insId": "iqrfgd2-default"
    }
})";
    EXPECT_EQ(expected, received);
    // clear buffer
    buffer.consume(buffer.size());

    // revoke token
    revokeToken(revoked_later_token.getId());
    // Read revoked token message message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":7,"error":"Revoked token","type":"auth_error"})");
    // clear buffer
    buffer.consume(buffer.size());
    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::policy_error);
  }

  TEST_F(WebsocketMessagingAuthTest, test_websocket_messaging_auth_success_deleted_after) {
    beast::error_code ec;

    // do successful auth
    json doc({
      {"auth", valid_token_string}
    });
    ws.write(net::buffer(doc.dump()), ec);
    ASSERT_FALSE(ec);
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
    beast::flat_buffer buffer;
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    auto received = beast::buffers_to_string(buffer.data());
    std::string expected =
R"({
    "mType": "iqrfRaw",
    "data": {
        "msgId": "auth_success_test",
        "rsp": {
            "rData": "00.00.06.83.00.00.00.44"
        },
        "status": 0,
        "insId": "iqrfgd2-default"
    }
})";
    EXPECT_EQ(expected, received);
    // clear buffer
    buffer.consume(buffer.size());

    // revoke token
    removeToken(valid_token.getId());
    // Read revoked token message message
    ws.read(buffer, ec);
    ASSERT_FALSE(ec);
    received = beast::buffers_to_string(buffer.data());
    EXPECT_EQ(received, R"({"code":4,"error":"Token not found","type":"auth_error"})");
    // clear buffer
    buffer.consume(buffer.size());
    // Read close frame and check close reason
    ws.read(buffer, ec);
    EXPECT_EQ(ec, websocket::error::closed);
    EXPECT_EQ(ws.reason().code, websocket::close_code::internal_error);
  }

}
