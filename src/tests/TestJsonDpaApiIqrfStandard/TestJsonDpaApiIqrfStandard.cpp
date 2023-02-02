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

#define IIqrfChannelService_EXPORTS

#include "TestJsonDpaApiIqrfStandard.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "HexStringCoversion.h"

#include "gtest/gtest.h"

#include "rapidjson/pointer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "iqrf__TestJsonDpaApiIqrfStandard.hxx"

TRC_INIT_MNAME(iqrf::TestJsonDpaApiIqrfStandard)

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
        "TestJsonDpaApiIqrfStandard instance activate" << std::endl <<
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
        "TestJsonDpaApiIqrfStandard instance deactivate" << std::endl <<
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

    void detachInterface(shape::ILaunchService* iface)
    {
      if (m_iLaunchService == iface) {
        m_iLaunchService = nullptr;
      }
    }

  };

  ////////////////////////////////////
  TestJsonDpaApiIqrfStandard::TestJsonDpaApiIqrfStandard()
  {
  }

  TestJsonDpaApiIqrfStandard::~TestJsonDpaApiIqrfStandard()
  {
  }

  void TestJsonDpaApiIqrfStandard::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestJsonDpaApiIqrfStandard::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestJsonDpaApiIqrfStandard::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void TestJsonDpaApiIqrfStandard::attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonDpaApiIqrfStandard::detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonDpaApiIqrfStandard::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonDpaApiIqrfStandard::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonDpaApiIqrfStandard::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonDpaApiIqrfStandard::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonDpaApiIqrfStandard::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestJsonDpaApiIqrfStandard::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class JsonDpaApiIqrfStandardTesting : public ::testing::Test
  {
  protected:

    void SetUp(void) override
    {
      ASSERT_NE(nullptr, &Imp::get().m_iLaunchService);
    };

    void TearDown(void) override
    {
    };

    //for debug only
    static std::string JsonToStr(const rapidjson::Value* val)
    {
      rapidjson::Document doc;
      doc.CopyFrom(*val, doc.GetAllocator());
      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      return buffer.GetString();
    }
  };

  /////////// Tests
  //TODO missing:
  // parameters shall be taken from .../iqrf-daemon/tests/test-api-app/test-api-app/log/
  // and provided to parametrisation of TEST_F(JsonDpaApiIqrfStandardTesting, iqrfEmbedCoordinator_AddrInfo) example

  const unsigned MILLIS_WAIT = 1000;

#if 0
  //TODO move to IqrfDpa testing
  TEST_F(JsonDpaApiIqrfStandardTesting, IqrfDpaActivate)
  {
    //simulate send async response after TR reset
    Imp::get().m_accessControl.messageHandler(DotMsg("00.00.ff.3f.00.00.00.00.28.02.00.fd.26.00.00.00.00.00.00.01"));
    EXPECT_EQ("00.00.02.00.ff.ff", Imp::get().fetchMessage(MILLIS_WAIT));

    //simulate send OS read response
    Imp::get().m_accessControl.messageHandler(DotMsg("00.00.02.80.00.00.00.00.8a.52.00.81.38.24.79.08.00.28.00.c0"));
    EXPECT_EQ(ATTACH_IqrfDpa, Imp::get().fetchMessage(MILLIS_WAIT*2));

    auto par = Imp::get().m_iIqrfDpaService->getCoordinatorParameters();
    TRC_DEBUG(PAR(par.demoFlag) << PAR(par.dpaVer) << PAR(par.dpaVerMajor) << PAR(par.dpaVerMinor) <<
      PAR(par.lpModeSupportFlag) << PAR(par.stdModeSupportFlag) << std::endl <<
      PAR(par.mcuType) << PAR(par.moduleId) << PAR(par.osBuild) << PAR(par.osVersion) );

    EXPECT_EQ(false, par.demoFlag);
    EXPECT_EQ("02.28", par.dpaVer);
    EXPECT_EQ(2, par.dpaVerMajor);
    EXPECT_EQ(28, par.dpaVerMinor);
    EXPECT_EQ(false, par.lpModeSupportFlag);
    EXPECT_EQ(true, par.stdModeSupportFlag);
    EXPECT_EQ("PIC16F1938", par.mcuType);
    EXPECT_EQ("8100528a", par.moduleId);
    EXPECT_EQ("0879", par.osBuild);
    EXPECT_EQ(" 3.08D", par.osVersion);
  }
#endif

  TEST_F(JsonDpaApiIqrfStandardTesting, iqrfEmbedCoordinator_AddrInfo)
  {
    //JSON message input (jmi) as received from a messaging
    std::string jmi =
      "{"
      "  \"mType\": \"iqrfEmbedCoordinator_AddrInfo\","
      "  \"data\" : {"
      "    \"msgId\": \"testEmbedCoordinator\","
      "    \"timeout\" : 1000,"
      "    \"req\" : {"
      "      \"nAdr\": 0,"
      "      \"param\" : {}"
      "    },"
      "    \"returnVerbose\" : true"
      "  }"
      "}";

    //simulate receiving of jmi
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(jmi);

    //expected DPA transaction as result jmi processing
    //expected DPA request sent from IqrfDpa
    EXPECT_EQ("00.00.00.00.ff.ff", Imp::get().m_iTestSimulationIqrfChannel->popIncomingMessage(MILLIS_WAIT));
    //simulate send DPA response
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage("00.00.00.80.00.00.00.40.04.2a", 10);

    //expected JSON message output (jmo) as result of processing to be sent out by a messaging
    std::string jmo = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);

    //TODO check EXPECT jmo
    // parse jmo and replace timestamps as the time cannot be compared
    // in tests with timestams from .../iqrf-daemon\tests\test-api-app/test-api-app/log/
    // after timestamps homogenization it shall be possible to compare jmo as string

    //just logout for now
    TRC_DEBUG(jmo);
  }
}
