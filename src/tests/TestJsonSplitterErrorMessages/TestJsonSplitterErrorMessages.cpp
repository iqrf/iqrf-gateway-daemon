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

#include "TestJsonSplitterErrorMessages.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "HexStringCoversion.h"
#include "TestUtils.h"

#include "gtest/gtest.h"

#include "iqrf__TestJsonSplitterErrorMessages.hxx"

TRC_INIT_MNAME(iqrf::TestJsonSplitterErrorMessages)

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
        "TestJsonSplitterErrorMessages instance activate" << std::endl <<
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
        "TestJsonSplitterErrorMessages instance deactivate" << std::endl <<
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
  TestJsonSplitterErrorMessages::TestJsonSplitterErrorMessages()
  {
  }

  TestJsonSplitterErrorMessages::~TestJsonSplitterErrorMessages()
  {
  }

  void TestJsonSplitterErrorMessages::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestJsonSplitterErrorMessages::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestJsonSplitterErrorMessages::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void TestJsonSplitterErrorMessages::attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonSplitterErrorMessages::detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonSplitterErrorMessages::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonSplitterErrorMessages::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonSplitterErrorMessages::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonSplitterErrorMessages::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonSplitterErrorMessages::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestJsonSplitterErrorMessages::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class JsonSplitterErrorMessagesTest : public ::testing::Test {};

  TEST_F(JsonSplitterErrorMessagesTest, json_parse_error) {
    std::string request = R"({
      "mType: "invalid_json"
    })";

    // Send request and get error response
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(request);
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());

    EXPECT_STREQ("messageError", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("unknown", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(request, Pointer("/data/rsp/message").Get(doc)->GetString());
    EXPECT_STREQ("Missing a colon after a name of object member.", Pointer("/data/rsp/error").Get(doc)->GetString());
    EXPECT_EQ(17, Pointer("/data/rsp/offset").Get(doc)->GetInt());
    EXPECT_EQ(2, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Failed to parse JSON message.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(JsonSplitterErrorMessagesTest, missing_mtype_error) {
    std::string request = R"({
      "data": {
        "msgId": "test_missing_mtype",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.FF.FF"
        },
        "returnVerbose": true
      }
    })";

    // Send request and get error response
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(request);
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());

    EXPECT_STREQ("messageError", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_missing_mtype", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(request, Pointer("/data/rsp/message").Get(doc)->GetString());
    EXPECT_EQ(3, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("mType missing in JSON message.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(JsonSplitterErrorMessagesTest, invalid_message_error) {
    std::string request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "test_invalid_message",
        "timeout": 1000,
        "req": {},
        "returnVerbose": true
      }
    })";

    // Send request and get error response
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(request);
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());

    EXPECT_STREQ("messageError", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_invalid_message", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(request, Pointer("/data/rsp/message").Get(doc)->GetString());
    EXPECT_STREQ("Failed to validate request message. Violating member: <root>[data][req]. Violation: Missing required property 'rData'.", Pointer("/data/rsp/error").Get(doc)->GetString());
    EXPECT_EQ(4, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Failed to validate JSON message contents.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(JsonSplitterErrorMessagesTest, unexpected_auth_error) {
    std::string request = R"({
      "type": "auth",
      "token": "iqrfgd2;1;zDrcvQaXWopzJ+DbfkpGq3Tn00wkt3n6fExj8iUsYio="
    })";

    // Send request and get error response
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(request);
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());

    EXPECT_STREQ("messageError", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("auth", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("Received a duplicate or unexpected auth message.", Pointer("/data/rsp/error").Get(doc)->GetString());
    EXPECT_EQ(9, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Unexpected auth message.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(JsonSplitterErrorMessagesTest, service_mode_active_error) {
    // enable service mode
    std::string modeRequest = R"({
      "mType": "mngDaemon_Mode",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "req": {
          "operMode": "service"
        },
        "returnVerbose": true
      }
    })";
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(modeRequest);
    std::string modeResponse = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(100);
    auto doc = parseJsonString(modeResponse);
    ASSERT_FALSE(doc.HasParseError());
    // check success response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("service", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
    // attempt non-service message
    std::string rawRequest = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "test_raw",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
        },
        "returnVerbose": true
      }
    })";
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(rawRequest);
    std::string rawResponse = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(100);
    doc = parseJsonString(rawResponse);
    ASSERT_FALSE(doc.HasParseError());
    // check error response
    EXPECT_STREQ("messageError", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_raw", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_EQ(10, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Service mode is active.", Pointer("/data/statusStr").Get(doc)->GetString());
    // disable service mode
    modeRequest = R"({
      "mType": "mngDaemon_Mode",
      "data": {
        "msgId": "d6b05c55-408b-459d-bc25-f74c42fa0153",
        "req": {
          "operMode": "operational"
        },
        "returnVerbose": true
      }
    })";
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(modeRequest);
    modeResponse = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(100);
    doc = parseJsonString(modeResponse);
    ASSERT_FALSE(doc.HasParseError());
    // check success response
    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("d6b05c55-408b-459d-bc25-f74c42fa0153", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("operational", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("ok", Pointer("/data/statusStr").Get(doc)->GetString());
  }
}
