/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define IIqrfChannelService_EXPORTS

#include "TestJsonDpaApiRaw.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "HexStringCoversion.h"

#include "gtest/gtest.h"

#include "rapidjson/pointer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "iqrf__TestJsonDpaApiRaw.hxx"

TRC_INIT_MNAME(iqrf::TestJsonDpaApiRaw)

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
        "TestJsonDpaApiRaw instance activate" << std::endl <<
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
        "TestJsonDpaApiRaw instance deactivate" << std::endl <<
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
  TestJsonDpaApiRaw::TestJsonDpaApiRaw()
  {
  }

  TestJsonDpaApiRaw::~TestJsonDpaApiRaw()
  {
  }

  void TestJsonDpaApiRaw::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestJsonDpaApiRaw::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestJsonDpaApiRaw::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void TestJsonDpaApiRaw::attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonDpaApiRaw::detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonDpaApiRaw::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonDpaApiRaw::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonDpaApiRaw::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonDpaApiRaw::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonDpaApiRaw::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestJsonDpaApiRaw::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class JsonDpaApiRawTesting : public ::testing::Test
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

  const unsigned MILLIS_WAIT = 1000;

  TEST_F(JsonDpaApiRawTesting, iqrfEmbedCoordinator_AddrInfo)
  {
    //JSON message input (jmi) as received from a messaging
    std::string jmoexp =
      "{\n    \"mType\": \"iqrfRaw\",\n    \"data\": {\n        \"msgId\": \"async\",\n        \"rsp\": {\n            \"rData\": \"00.00.ff.3f.00.00.00.00.28.02.00.fd.26.00.00.00.00.00.00.01\"\n        },\n        \"status\": 0,\n        \"insId\": \"iqrfgd2-default\"\n    }\n}";

    //simulate async DPA response
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage("00.00.ff.3f.00.00.00.00.28.02.00.fd.26.00.00.00.00.00.00.01", 10);

    //expected JSON message output (jmo) as result of processing to be sent out by a messaging
    std::string jmo = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);

    EXPECT_EQ(jmoexp, jmo);

    TRC_DEBUG(jmo);
  }
}
