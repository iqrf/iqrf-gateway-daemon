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
#include "TestServiceModeUdpDisabled.h"
#include "Trace.h"
#include "TestUtils.h"

#include "iqrf__TestServiceModeUdpDisabled.hxx"

#include "gtest/gtest.h"

TRC_INIT_MNAME(iqrf::TestServiceModeUdpDisabled)

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
        "TestServiceModeUdpDisabled instance activate" << std::endl <<
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
        "TestServiceModeUdpDisabled instance deactivate" << std::endl <<
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
  TestServiceModeUdpDisabled::TestServiceModeUdpDisabled()
  {
  }

  TestServiceModeUdpDisabled::~TestServiceModeUdpDisabled()
  {
  }

  void TestServiceModeUdpDisabled::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestServiceModeUdpDisabled::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestServiceModeUdpDisabled::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void TestServiceModeUdpDisabled::attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestServiceModeUdpDisabled::detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestServiceModeUdpDisabled::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestServiceModeUdpDisabled::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestServiceModeUdpDisabled::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestServiceModeUdpDisabled::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestServiceModeUdpDisabled::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestServiceModeUdpDisabled::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  //////////////////////////////////////////////////////////////////
  class ServiceModeUdpDisabledTest : public ::testing::Test {};

  TEST_F(ServiceModeUdpDisabledTest, set_service_udp_not_active) {
    std::string request = R"({
      "mType": "mngDaemon_Mode",
      "data": {
        "msgId": "test_set_service_mode",
        "req": {
          "operMode": "service"
        },
        "returnVerbose": true
      }
    })";

    // Send request and get error response
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(request);
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());

    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_set_service_mode", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("operational", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_STREQ("UDP service not active.", Pointer("/data/errorStr").Get(doc)->GetString());
    EXPECT_EQ(-1, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("err", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeUdpDisabledTest, set_operational_udp_not_active) {
    std::string request = R"({
      "mType": "mngDaemon_Mode",
      "data": {
        "msgId": "test_set_operational_mode",
        "req": {
          "operMode": "operational"
        },
        "returnVerbose": true
      }
    })";

    // Send request and get error response
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(request);
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());

    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_set_operational_mode", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("operational", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_STREQ("UDP service not active.", Pointer("/data/errorStr").Get(doc)->GetString());
    EXPECT_EQ(-1, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("err", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeUdpDisabledTest, set_forwarding_udp_not_active) {
    std::string request = R"({
      "mType": "mngDaemon_Mode",
      "data": {
        "msgId": "test_set_forwarding_mode",
        "req": {
          "operMode": "forwarding"
        },
        "returnVerbose": true
      }
    })";

    // Send request and get error response
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(request);
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());

    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_set_forwarding_mode", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("operational", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_STREQ("UDP service not active.", Pointer("/data/errorStr").Get(doc)->GetString());
    EXPECT_EQ(-1, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("err", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(ServiceModeUdpDisabledTest, get_mode_udp_not_active) {
    std::string request = R"({
      "mType": "mngDaemon_Mode",
      "data": {
        "msgId": "test_get_mode",
        "req": {
          "operMode": ""
        },
        "returnVerbose": true
      }
    })";

    // Send request and get error response
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(request);
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());

    EXPECT_STREQ("mngDaemon_Mode", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_get_mode", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("operational", Pointer("/data/rsp/operMode").Get(doc)->GetString());
    EXPECT_STREQ("UDP service not active.", Pointer("/data/errorStr").Get(doc)->GetString());
    EXPECT_EQ(-1, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("err", Pointer("/data/statusStr").Get(doc)->GetString());
  }
}
