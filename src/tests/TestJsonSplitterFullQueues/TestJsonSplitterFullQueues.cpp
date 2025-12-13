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

#include "TestJsonSplitterFullQueues.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "HexStringCoversion.h"
#include "TestUtils.h"

#include "gtest/gtest.h"

#include "iqrf__TestJsonSplitterFullQueues.hxx"

TRC_INIT_MNAME(iqrf::TestJsonSplitterFullQueues)

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
        "TestJsonSplitterFullQueues instance activate" << std::endl <<
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
        "TestJsonSplitterFullQueues instance deactivate" << std::endl <<
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
  TestJsonSplitterFullQueues::TestJsonSplitterFullQueues()
  {
  }

  TestJsonSplitterFullQueues::~TestJsonSplitterFullQueues()
  {
  }

  void TestJsonSplitterFullQueues::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestJsonSplitterFullQueues::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestJsonSplitterFullQueues::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void TestJsonSplitterFullQueues::attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonSplitterFullQueues::detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonSplitterFullQueues::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonSplitterFullQueues::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonSplitterFullQueues::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonSplitterFullQueues::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonSplitterFullQueues::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestJsonSplitterFullQueues::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class JsonSplitterFullQueueTest : public ::testing::Test {};

  TEST_F(JsonSplitterFullQueueTest, management_queue_full) {
    std::string request = R"({
      "mType": "mngDaemon_Version",
      "data": {
        "msgId": "test_full_management_queue",
        "returnVerbose": true
      }
    })";

    // Send request and get error response
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(request);
    std::string response = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
    auto doc = parseJsonString(response);
    ASSERT_FALSE(doc.HasParseError());

    EXPECT_STREQ("messageError", Pointer("/mType").Get(doc)->GetString());
    EXPECT_STREQ("test_full_management_queue", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("mngDaemon_Version", Pointer("/data/rsp/ignoredMessage").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/rsp/capacity").Get(doc)->GetInt());
    EXPECT_EQ(6, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Management queue is full.", Pointer("/data/statusStr").Get(doc)->GetString());
  }

  TEST_F(JsonSplitterFullQueueTest, network_queue_full) {
    std::string request = R"({
      "mType": "iqrfRaw",
      "data": {
        "msgId": "test_full_network_queue",
        "timeout": 1000,
        "req": {
          "rData": "00.00.06.03.ff.ff"
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
    EXPECT_STREQ("test_full_network_queue", Pointer("/data/msgId").Get(doc)->GetString());
    EXPECT_STREQ("iqrfRaw", Pointer("/data/rsp/ignoredMessage").Get(doc)->GetString());
    EXPECT_EQ(0, Pointer("/data/rsp/capacity").Get(doc)->GetInt());
    EXPECT_EQ(8, Pointer("/data/status").Get(doc)->GetInt());
    EXPECT_STREQ("Network queue is full.", Pointer("/data/statusStr").Get(doc)->GetString());
  }
}
