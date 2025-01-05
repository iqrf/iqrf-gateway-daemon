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
      (void)props; //silence -Wunused-parameter
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
    (void)props; //silence -Wunused-parameter
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
    std::string requestStr =
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
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(requestStr);

    // expected DPA request sent from IqrfDpa to Coordinator
    //std::string responseStr = Imp::get().m_iTestSimulationIqrfChannel->popIncomingMessage(MILLIS_WAIT);

    // simulate send DPA response
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.02.82.00.00.00.00.b4.c9.10.34.34.34.3c.34.32.32.32.37.34.34.34.34.34.15.1d.34.34.34.34.34.34.34.34.34.34.34.37.34.00.30",
      10
    );

    // expected JSON message output (jmo) as result of processing to be sent out by a messaging
    std::string responseStr = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);

    rapidjson::Document responseDoc;
    responseDoc.Parse(responseStr);

    // testing response data field values
    rapidjson::Value* msgMtypeJsonVal = rapidjson::Pointer("/mType").Get(responseDoc);
    EXPECT_NE(msgMtypeJsonVal, nullptr);
    EXPECT_STREQ(msgMtypeJsonVal->GetString(), "iqmeshNetwork_ReadTrConf");

    rapidjson::Value* msgIdJsonVal = rapidjson::Pointer("/data/msgId").Get(responseDoc);
    EXPECT_NE(msgIdJsonVal, nullptr);
    EXPECT_STREQ(msgIdJsonVal->GetString(), "testReadTrConf");

    rapidjson::Value* devAddrJsonVal = rapidjson::Pointer("/data/rsp/deviceAddr").Get(responseDoc);
    EXPECT_NE(devAddrJsonVal, nullptr);
    EXPECT_EQ(devAddrJsonVal->GetInt(), 0);


    // embedded peripheral presence - byte values
    rapidjson::Value* embPersJsonVal = rapidjson::Pointer("/data/rsp/embPers/values").Get(responseDoc);
    EXPECT_NE(embPersJsonVal, nullptr);

    if (!HasNonfatalFailure()) {
      EXPECT_TRUE(embPersJsonVal->IsArray());
      EXPECT_EQ(embPersJsonVal->Size(), (unsigned)4);

      // parsing and checking raw object
      EXPECT_EQ((*embPersJsonVal)[0].GetInt(), 253);
      EXPECT_EQ((*embPersJsonVal)[1].GetInt(), 36);
      EXPECT_EQ((*embPersJsonVal)[2].GetInt(), 0);
      EXPECT_EQ((*embPersJsonVal)[3].GetInt(), 0);
    }

    // embedded peripheral presence - parsed
    rapidjson::Value* coordJsonVal = rapidjson::Pointer("/data/rsp/embPers/coordinator").Get(responseDoc);
    EXPECT_NE(coordJsonVal, nullptr);
    EXPECT_EQ(coordJsonVal->GetBool(), true);

    rapidjson::Value* nodeJsonVal = rapidjson::Pointer("/data/rsp/embPers/node").Get(responseDoc);
    EXPECT_NE(nodeJsonVal, nullptr);
    EXPECT_EQ(nodeJsonVal->GetBool(), false);

    rapidjson::Value* osJsonVal = rapidjson::Pointer("/data/rsp/embPers/os").Get(responseDoc);
    EXPECT_NE(osJsonVal, nullptr);
    EXPECT_EQ(osJsonVal->GetBool(), true);

    rapidjson::Value* eepromJsonVal = rapidjson::Pointer("/data/rsp/embPers/eeprom").Get(responseDoc);
    EXPECT_NE(eepromJsonVal, nullptr);
    EXPECT_EQ(eepromJsonVal->GetBool(), true);

    rapidjson::Value* eeepromJsonVal = rapidjson::Pointer("/data/rsp/embPers/eeeprom").Get(responseDoc);
    EXPECT_NE(eeepromJsonVal, nullptr);
    EXPECT_EQ(eeepromJsonVal->GetBool(), true);

    rapidjson::Value* ramJsonVal = rapidjson::Pointer("/data/rsp/embPers/ram").Get(responseDoc);
    EXPECT_NE(ramJsonVal, nullptr);
    EXPECT_EQ(ramJsonVal->GetBool(), true);

    rapidjson::Value* ledrJsonVal = rapidjson::Pointer("/data/rsp/embPers/ledr").Get(responseDoc);
    EXPECT_NE(ledrJsonVal, nullptr);
    EXPECT_EQ(ledrJsonVal->GetBool(), true);

    rapidjson::Value* ledgJsonVal = rapidjson::Pointer("/data/rsp/embPers/ledg").Get(responseDoc);
    EXPECT_NE(ledgJsonVal, nullptr);
    EXPECT_EQ(ledgJsonVal->GetBool(), true);

    rapidjson::Value* spiJsonVal = rapidjson::Pointer("/data/rsp/embPers/spi").Get(responseDoc);
    EXPECT_NE(spiJsonVal, nullptr);
    EXPECT_EQ(spiJsonVal->GetBool(), false);

    rapidjson::Value* ioJsonVal = rapidjson::Pointer("/data/rsp/embPers/io").Get(responseDoc);
    EXPECT_NE(ioJsonVal, nullptr);
    EXPECT_EQ(ioJsonVal->GetBool(), false);

    rapidjson::Value* thermometerJsonVal = rapidjson::Pointer("/data/rsp/embPers/thermometer").Get(responseDoc);
    EXPECT_NE(thermometerJsonVal, nullptr);
    EXPECT_EQ(thermometerJsonVal->GetBool(), true);

    rapidjson::Value* pwmJsonVal = rapidjson::Pointer("/data/rsp/embPers/pwm").Get(responseDoc);
    EXPECT_NE(pwmJsonVal, nullptr);
    EXPECT_EQ(pwmJsonVal->GetBool(), false);

    rapidjson::Value* uartJsonVal = rapidjson::Pointer("/data/rsp/embPers/uart").Get(responseDoc);
    EXPECT_NE(uartJsonVal, nullptr);
    EXPECT_EQ(uartJsonVal->GetBool(), false);

    rapidjson::Value* frcJsonVal = rapidjson::Pointer("/data/rsp/embPers/frc").Get(responseDoc);
    EXPECT_NE(frcJsonVal, nullptr);
    EXPECT_EQ(frcJsonVal->GetBool(), true);


    // other fields read from HWP configuration block
    rapidjson::Value* customDpaHandlerJsonVal = rapidjson::Pointer("/data/rsp/customDpaHandler").Get(responseDoc);
    EXPECT_NE(customDpaHandlerJsonVal, nullptr);
    EXPECT_EQ(customDpaHandlerJsonVal->GetBool(), false);

    rapidjson::Value* nodeDpaInterfaceJsonVal = rapidjson::Pointer("/data/rsp/nodeDpaInterface").Get(responseDoc);
    EXPECT_NE(nodeDpaInterfaceJsonVal, nullptr);
    EXPECT_EQ(nodeDpaInterfaceJsonVal->GetBool(), false);

    rapidjson::Value* dpaAutoexecJsonVal = rapidjson::Pointer("/data/rsp/dpaAutoexec").Get(responseDoc);
    EXPECT_NE(dpaAutoexecJsonVal, nullptr);
    EXPECT_EQ(dpaAutoexecJsonVal->GetBool(), false);

    rapidjson::Value* routingOffJsonVal = rapidjson::Pointer("/data/rsp/routingOff").Get(responseDoc);
    EXPECT_NE(routingOffJsonVal, nullptr);
    EXPECT_EQ(routingOffJsonVal->GetBool(), false);

    rapidjson::Value* ioSetupJsonVal = rapidjson::Pointer("/data/rsp/ioSetup").Get(responseDoc);
    EXPECT_NE(ioSetupJsonVal, nullptr);
    EXPECT_EQ(ioSetupJsonVal->GetBool(), false);

    rapidjson::Value* peerToPeerJsonVal = rapidjson::Pointer("/data/rsp/peerToPeer").Get(responseDoc);
    EXPECT_NE(peerToPeerJsonVal, nullptr);
    EXPECT_EQ(peerToPeerJsonVal->GetBool(), false);

    rapidjson::Value* rfBandJsonVal = rapidjson::Pointer("/data/rsp/rfBand").Get(responseDoc);
    EXPECT_NE(rfBandJsonVal, nullptr);
    EXPECT_STREQ(rfBandJsonVal->GetString(), "868");

    rapidjson::Value* rfChannelAJsonVal = rapidjson::Pointer("/data/rsp/rfChannelA").Get(responseDoc);
    EXPECT_NE(rfChannelAJsonVal, nullptr);
    EXPECT_EQ(rfChannelAJsonVal->GetInt(), 33);

    rapidjson::Value* rfChannelBJsonVal = rapidjson::Pointer("/data/rsp/rfChannelB").Get(responseDoc);
    EXPECT_NE(rfChannelBJsonVal, nullptr);
    EXPECT_EQ(rfChannelBJsonVal->GetInt(), 41);

    rapidjson::Value* rfSubChannelAJsonVal = rapidjson::Pointer("/data/rsp/rfSubChannelA").Get(responseDoc);
    EXPECT_NE(rfSubChannelAJsonVal, nullptr);
    EXPECT_EQ(rfSubChannelAJsonVal->GetInt(), 8);

    rapidjson::Value* rfSubChannelBJsonVal = rapidjson::Pointer("/data/rsp/rfSubChannelB").Get(responseDoc);
    EXPECT_NE(rfSubChannelBJsonVal, nullptr);
    EXPECT_EQ(rfSubChannelBJsonVal->GetInt(), 0);

    rapidjson::Value* txPowerJsonVal = rapidjson::Pointer("/data/rsp/txPower").Get(responseDoc);
    EXPECT_NE(txPowerJsonVal, nullptr);
    EXPECT_EQ(txPowerJsonVal->GetInt(), 6);

    rapidjson::Value* rxFilterJsonVal = rapidjson::Pointer("/data/rsp/rxFilter").Get(responseDoc);
    EXPECT_NE(rxFilterJsonVal, nullptr);
    EXPECT_EQ(rxFilterJsonVal->GetInt(), 6);

    rapidjson::Value* lpRxTimeoutJsonVal = rapidjson::Pointer("/data/rsp/lpRxTimeout").Get(responseDoc);
    EXPECT_NE(lpRxTimeoutJsonVal, nullptr);
    EXPECT_EQ(lpRxTimeoutJsonVal->GetInt(), 6);

    rapidjson::Value* rfPgmAltChannelJsonVal = rapidjson::Pointer("/data/rsp/rfPgmAltChannel").Get(responseDoc);
    EXPECT_NE(rfPgmAltChannelJsonVal, nullptr);
    EXPECT_EQ(rfPgmAltChannelJsonVal->GetInt(), 0);

    rapidjson::Value* uartBaudrateJsonVal = rapidjson::Pointer("/data/rsp/uartBaudrate").Get(responseDoc);
    EXPECT_NE(uartBaudrateJsonVal, nullptr);
    EXPECT_EQ(uartBaudrateJsonVal->GetInt(), 9600);


    // RFPGM subfields
    rapidjson::Value* rfPgmDualChannelJsonVal = rapidjson::Pointer("/data/rsp/rfPgmDualChannel").Get(responseDoc);
    EXPECT_NE(rfPgmDualChannelJsonVal, nullptr);
    EXPECT_EQ(rfPgmDualChannelJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmLpModeJsonVal = rapidjson::Pointer("/data/rsp/rfPgmLpMode").Get(responseDoc);
    EXPECT_NE(rfPgmLpModeJsonVal, nullptr);
    EXPECT_EQ(rfPgmLpModeJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmIncorrectUploadJsonVal = rapidjson::Pointer("/data/rsp/rfPgmIncorrectUpload").Get(responseDoc);
    EXPECT_NE(rfPgmIncorrectUploadJsonVal, nullptr);
    EXPECT_EQ(rfPgmIncorrectUploadJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmEnableAfterResetJsonVal = rapidjson::Pointer("/data/rsp/rfPgmEnableAfterReset").Get(responseDoc);
    EXPECT_NE(rfPgmEnableAfterResetJsonVal, nullptr);
    EXPECT_EQ(rfPgmEnableAfterResetJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmTerminateAfter1MinJsonVal = rapidjson::Pointer("/data/rsp/rfPgmTerminateAfter1Min").Get(responseDoc);
    EXPECT_NE(rfPgmTerminateAfter1MinJsonVal, nullptr);
    EXPECT_EQ(rfPgmTerminateAfter1MinJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmTerminateMcuPinJsonVal = rapidjson::Pointer("/data/rsp/rfPgmTerminateMcuPin").Get(responseDoc);
    EXPECT_NE(rfPgmTerminateMcuPinJsonVal, nullptr);
    EXPECT_EQ(rfPgmTerminateMcuPinJsonVal->GetBool(), false);


    // raw data
    rapidjson::Value* rawDataJson = rapidjson::Pointer("/data/raw").Get(responseDoc);
    EXPECT_NE(rawDataJson, nullptr);

    if (!HasNonfatalFailure()) {
      EXPECT_EQ(rawDataJson->IsArray(), true);
      EXPECT_EQ(rawDataJson->Size(), (unsigned)1);

      // parsing and checking raw object
      EXPECT_EQ((*rawDataJson)[0].IsObject(), true);

      // request
      rapidjson::Value::MemberIterator requestIter = (*rawDataJson)[0].FindMember("request");
      EXPECT_NE(requestIter, (*rawDataJson)[0].MemberEnd());
      EXPECT_STREQ(requestIter->value.GetString(), "00.00.02.02.ff.ff");

      // confirmation
      rapidjson::Value::MemberIterator confirmIter = (*rawDataJson)[0].FindMember("confirmation");
      EXPECT_NE(confirmIter, (*rawDataJson)[0].MemberEnd());
      EXPECT_STREQ(confirmIter->value.GetString(), "");

      // response
      rapidjson::Value::MemberIterator responseIter = (*rawDataJson)[0].FindMember("response");
      EXPECT_NE(responseIter, (*rawDataJson)[0].MemberEnd());
      EXPECT_STREQ(
        responseIter->value.GetString(),
        "00.00.02.82.00.00.00.00.b4.c9.10.34.34.34.3c.34.32.32.32.37.34.34.34.34.34.15.1d.34.34.34.34.34.34.34.34.34.34.34.37.34.00.30"
      );
    }

    // status
    rapidjson::Value* statusJsonVal = rapidjson::Pointer("/data/status").Get(responseDoc);
    EXPECT_NE(statusJsonVal, nullptr);
    EXPECT_EQ(statusJsonVal->GetInt(), 0);

    rapidjson::Value* statusStr = rapidjson::Pointer("/data/statusStr").Get(responseDoc);
    EXPECT_NE(statusStr, nullptr);
    EXPECT_STREQ(statusStr->GetString(), "ok");
  }

  // invalid device address
  TEST_F(ReadTrConfTesting, iqmeshReadTrConfService_2)
  {
    // JSON request - as received from messaging
    std::string requestStr =
      "{"
      "  \"mType\": \"iqmeshNetwork_ReadTrConf\","
      "  \"data\" : {"
      "    \"msgId\": \"testReadTrConf\","
      "    \"timeout\" : 1000,"
      "    \"req\" : {"
      "      \"deviceAddr\": -1"
      "    },"
      "    \"returnVerbose\" : true"
      "  }"
      "}";

    // simulate receiving of request by splitter and tested service
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(requestStr);

    // NOTE:
    // read Tr Conf Service should send error JSON response WITHOUT ever sending some DPA request

    // expected JSON message output (jmo) as result of processing to be sent out by a messaging
    std::string responseStr = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);

    rapidjson::Document responseDoc;
    responseDoc.Parse(responseStr);


    // testing response data field values
    rapidjson::Value* msgMtypeJsonVal = rapidjson::Pointer("/mType").Get(responseDoc);
    EXPECT_NE(msgMtypeJsonVal, nullptr);
    EXPECT_STREQ(msgMtypeJsonVal->GetString(), "iqmeshNetwork_ReadTrConf");

    rapidjson::Value* msgIdJsonVal = rapidjson::Pointer("/data/msgId").Get(responseDoc);
    EXPECT_NE(msgIdJsonVal, nullptr);
    EXPECT_STREQ(msgIdJsonVal->GetString(), "testReadTrConf");

    // address should be NOT present in the response
    rapidjson::Value* devAddrJsonVal = rapidjson::Pointer("/data/rsp/deviceAddr").Get(responseDoc);
    EXPECT_EQ(devAddrJsonVal, nullptr);


    // status
    rapidjson::Value* statusJsonVal = rapidjson::Pointer("/data/status").Get(responseDoc);
    EXPECT_NE(statusJsonVal, nullptr);
    EXPECT_EQ(statusJsonVal->GetInt(), 1000);

    rapidjson::Value* statusStr = rapidjson::Pointer("/data/statusStr").Get(responseDoc);
    EXPECT_NE(statusStr, nullptr);
    EXPECT_STRNE(statusStr->GetString(), "ok");
  }

  // invalid device address
  TEST_F(ReadTrConfTesting, iqmeshReadTrConfService_3)
  {
    // JSON request - as received from messaging
    std::string requestStr =
      "{"
      "  \"mType\": \"iqmeshNetwork_ReadTrConf\","
      "  \"data\" : {"
      "    \"msgId\": \"testReadTrConf\","
      "    \"timeout\" : 1000,"
      "    \"req\" : {"
      "      \"deviceAddr\": 240"
      "    },"
      "    \"returnVerbose\" : true"
      "  }"
      "}";

    // simulate receiving of request by splitter and tested service
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(requestStr);

    // NOTE:
    // read Tr Conf Service should send error JSON response WITHOUT ever sending some DPA request

    // expected JSON message output (jmo) as result of processing to be sent out by a messaging
    std::string responseStr = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);

    rapidjson::Document responseDoc;
    responseDoc.Parse(responseStr);


    // testing response data field values
    rapidjson::Value* msgMtypeJsonVal = rapidjson::Pointer("/mType").Get(responseDoc);
    EXPECT_NE(msgMtypeJsonVal, nullptr);
    EXPECT_STREQ(msgMtypeJsonVal->GetString(), "iqmeshNetwork_ReadTrConf");

    rapidjson::Value* msgIdJsonVal = rapidjson::Pointer("/data/msgId").Get(responseDoc);
    EXPECT_NE(msgIdJsonVal, nullptr);
    EXPECT_STREQ(msgIdJsonVal->GetString(), "testReadTrConf");

    // address should be NOT present in the response
    rapidjson::Value* devAddrJsonVal = rapidjson::Pointer("/data/rsp/deviceAddr").Get(responseDoc);
    EXPECT_EQ(devAddrJsonVal, nullptr);


    // status
    rapidjson::Value* statusJsonVal = rapidjson::Pointer("/data/status").Get(responseDoc);
    EXPECT_NE(statusJsonVal, nullptr);
    EXPECT_EQ(statusJsonVal->GetInt(), 1000);

    rapidjson::Value* statusStr = rapidjson::Pointer("/data/statusStr").Get(responseDoc);
    EXPECT_NE(statusStr, nullptr);
    EXPECT_STRNE(statusStr->GetString(), "ok");
  }
}
