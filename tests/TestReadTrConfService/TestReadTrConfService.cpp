#define IIqrfChannelService_EXPORTS

#include "TestReadTrConfService.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "HexStringCoversion.h"

#include "gtest/gtest.h"

#include "rapidjson/pointer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "iqrf__TestReadTrConfService.hxx"

TRC_INIT_MNAME(iqrf::TestReadTrConfService)

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
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestReadTrConfService instance activate" << std::endl <<
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
        "TestReadTrConfService instance deactivate" << std::endl <<
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
  TestReadTrConfService::TestReadTrConfService()
  {
  }

  TestReadTrConfService::~TestReadTrConfService()
  {
  }

  void TestReadTrConfService::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestReadTrConfService::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestReadTrConfService::modify(const shape::Properties *props)
  {
  }

  void TestReadTrConfService::attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestReadTrConfService::detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestReadTrConfService::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestReadTrConfService::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestReadTrConfService::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestReadTrConfService::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestReadTrConfService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestReadTrConfService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class ReadTrConfTesting : public ::testing::Test
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
  // and provided to parametrisation of TEST_F(ReadTrConfTesting, iqrfEmbedCoordinator_AddrInfo) example

  const unsigned MILLIS_WAIT = 1000;

  TEST_F(ReadTrConfTesting, iqmeshReadTrConfService_1)
  {
    // JSON request - as received from messaging
    std::string jmi =
      "{"
      "  \"mType\": \"iqmeshNetwork_ReadTrConf\","
      "  \"data\" : {"
      "    \"msgId\": \"testReadTrConf\","
      "    \"timeout\" : 1000,"
      "    \"req\" : {"
      "      \"deviceAddr\": 0"
      "    },"
      "    \"returnVerbose\" : true"
      "  }"
      "}";

    // simulate receiving of request by splitter and tested service
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(jmi);
    
    // expected DPA request sent from IqrfDpa to Coordinator
    //std::string responseStr = Imp::get().m_iTestSimulationIqrfChannel->popIncomingMessage(MILLIS_WAIT);

    // simulate send DPA response
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.02.82.00.00.00.00.b4.c9.10.34.34.34.3c.34.32.32.32.37.34.34.34.34.34.15.1d.34.34.34.34.34.34.34.34.34.34.34.37.34.00.30", 
      10
    );
    
    // expected JSON message output (jmo) as result of processing to be sent out by a messaging
    std::string jmo = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);

    TRC_INFORMATION("Response = " << jmo);
    
    //just logout for now
    TRC_DEBUG(jmo);
  }
}
