#define IIqrfChannelService_EXPORTS

#include "TestEnumerationDeviceService.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "HexStringCoversion.h"

#include "gtest/gtest.h"

#include "rapidjson/pointer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "iqrf__TestEnumerationDeviceService.hxx"

TRC_INIT_MNAME(iqrf::TestEnumerationDeviceService)

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
        "TestEnumerationDeviceService instance activate" << std::endl <<
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
        "TestEnumerationDeviceService instance deactivate" << std::endl <<
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
  TestEnumerationDeviceService::TestEnumerationDeviceService()
  {
  }

  TestEnumerationDeviceService::~TestEnumerationDeviceService()
  {
  }

  void TestEnumerationDeviceService::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestEnumerationDeviceService::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestEnumerationDeviceService::modify(const shape::Properties *props)
  {
    (void)props; //silence -Wunused-parameter
  }

  void TestEnumerationDeviceService::attachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestEnumerationDeviceService::detachInterface(iqrf::ITestSimulationIqrfChannel* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestEnumerationDeviceService::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestEnumerationDeviceService::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestEnumerationDeviceService::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestEnumerationDeviceService::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestEnumerationDeviceService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestEnumerationDeviceService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class DeviceEnumerationTesting : public ::testing::Test
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
  
  TEST_F(DeviceEnumerationTesting, iqmeshNetwork_EnumerateDevice_1)
  {
    // JSON request - as received from messaging
    std::string requestStr =
    "{"
    "  \"mType\": \"iqmeshNetwork_EnumerateDevice\","
    "  \"data\" : {"
    "    \"msgId\": \"testEnumerateDevice\","
    "    \"timeout\" : 1000,"
    "    \"req\" : {"
    "      \"deviceAddr\": 0"
    "    },"
    "    \"returnVerbose\" : true"
    "  }"
    "}";

    // simulate receiving of request by splitter and tested service
    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(requestStr);

    // simulate send DPA response
    // Discovery data responses
    
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.00.8a.00.00.00.00.3f.3a.03.1d.0e.32.a1.08.03.1d.0b.32.ef.30.29.00.a7.00.ee.30.a8.00.ef.30.a9.00.ee.30.aa.00.03.14.08.00.03.10.29.00.08.00.00.00.00.00.00.00.00.00", 
      10
    );

    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.00.8a.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00",
      10
    );

    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.00.8a.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00",
      100
    );

    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.00.8a.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00",
      100
    );


    // OS read data response
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.02.80.00.00.00.00.dc.9c.00.81.40.24.91.08.00.28.05.31",
      100
    );

    // peripheral enumeration data response
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.ff.bf.00.00.00.00.02.03.00.fd.24.00.00.00.00.00.00.01",
      100
    );

    // read HWP configuration data response
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.02.82.00.00.00.00.b4.c9.10.34.34.34.3c.34.32.32.32.37.34.34.34.34.34.15.1d.34.34.34.34.34.34.34.34.34.34.34.37.34.00.30",
      100
    );

    // info for more peripherals data response
    Imp::get().m_iTestSimulationIqrfChannel->pushOutgoingMessage(
      "00.00.ff.bf.00.00.00.00.02.03.00.fd.24.00.00.00.00.00.00.01",
      100
    );
    
    // expected JSON message output (jmo) as result of processing to be sent out by a messaging
    std::string responseStr = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);

    rapidjson::Document responseDoc;
    responseDoc.Parse(responseStr);

    // testing response data field values
    rapidjson::Value* msgMtypeJsonVal = rapidjson::Pointer("/mType").Get(responseDoc);
    EXPECT_NE(msgMtypeJsonVal, nullptr);
    EXPECT_STREQ(msgMtypeJsonVal->GetString(), "iqmeshNetwork_EnumerateDevice");

    rapidjson::Value* msgIdJsonVal = rapidjson::Pointer("/data/msgId").Get(responseDoc);
    EXPECT_NE(msgIdJsonVal, nullptr);
    EXPECT_STREQ(msgIdJsonVal->GetString(), "testEnumerateDevice");

    rapidjson::Value* devAddrJsonVal = rapidjson::Pointer("/data/rsp/deviceAddr").Get(responseDoc);
    EXPECT_NE(devAddrJsonVal, nullptr);
    EXPECT_EQ(devAddrJsonVal->GetInt(), 0);


    // discovery
    rapidjson::Value* discoveredJsonVal = rapidjson::Pointer("/data/rsp/discovery/discovered").Get(responseDoc);
    EXPECT_NE(discoveredJsonVal, nullptr);
    EXPECT_EQ(discoveredJsonVal->GetBool(), true);

    rapidjson::Value* vrnJsonVal = rapidjson::Pointer("/data/rsp/discovery/vrn").Get(responseDoc);
    EXPECT_NE(vrnJsonVal, nullptr);
    EXPECT_EQ(vrnJsonVal->GetInt(), 0);

    rapidjson::Value* zoneJsonVal = rapidjson::Pointer("/data/rsp/discovery/zone").Get(responseDoc);
    EXPECT_NE(zoneJsonVal, nullptr);
    EXPECT_EQ(zoneJsonVal->GetInt(), 0);

    rapidjson::Value* parentJsonVal = rapidjson::Pointer("/data/rsp/discovery/parent").Get(responseDoc);
    EXPECT_NE(parentJsonVal, nullptr);
    EXPECT_EQ(parentJsonVal->GetInt(), 0);


    // OS read
    rapidjson::Value* midJsonVal = rapidjson::Pointer("/data/rsp/osRead/mid").Get(responseDoc);
    EXPECT_NE(midJsonVal, nullptr);
    EXPECT_STREQ(midJsonVal->GetString(), "81009cdc");

    rapidjson::Value* osVersionJsonVal = rapidjson::Pointer("/data/rsp/osRead/osVersion").Get(responseDoc);
    EXPECT_NE(osVersionJsonVal, nullptr);
    EXPECT_STREQ(osVersionJsonVal->GetString(), "4.00D");

    // TR MCU type
    rapidjson::Value* valueJsonVal = rapidjson::Pointer("/data/rsp/osRead/trMcuType/value").Get(responseDoc);
    EXPECT_NE(valueJsonVal, nullptr);
    EXPECT_EQ(valueJsonVal->GetInt(), 36);

    rapidjson::Value* trTypeJsonVal = rapidjson::Pointer("/data/rsp/osRead/trMcuType/trType").Get(responseDoc);
    EXPECT_NE(trTypeJsonVal, nullptr);
    EXPECT_STREQ(trTypeJsonVal->GetString(), "(DC)TR-72Dx");

    rapidjson::Value* fccCertifiedJsonVal = rapidjson::Pointer("/data/rsp/osRead/trMcuType/fccCertified").Get(responseDoc);
    EXPECT_NE(fccCertifiedJsonVal, nullptr);
    EXPECT_EQ(fccCertifiedJsonVal->GetBool(), false);

    rapidjson::Value* mcuTypeJsonVal = rapidjson::Pointer("/data/rsp/osRead/trMcuType/mcuType").Get(responseDoc);
    EXPECT_NE(mcuTypeJsonVal, nullptr);
    EXPECT_STREQ(mcuTypeJsonVal->GetString(), "PIC16LF1938");


    rapidjson::Value* osBuildJsonVal = rapidjson::Pointer("/data/rsp/osRead/osBuild").Get(responseDoc);
    EXPECT_NE(osBuildJsonVal, nullptr);
    EXPECT_STREQ(osBuildJsonVal->GetString(), "0891");

    rapidjson::Value* rssiJsonVal = rapidjson::Pointer("/data/rsp/osRead/rssi").Get(responseDoc);
    EXPECT_NE(rssiJsonVal, nullptr);
    EXPECT_STREQ(rssiJsonVal->GetString(), "126 dBm");

    rapidjson::Value* supplyVoltageJsonVal = rapidjson::Pointer("/data/rsp/osRead/supplyVoltage").Get(responseDoc);
    EXPECT_NE(supplyVoltageJsonVal, nullptr);
    EXPECT_STREQ(supplyVoltageJsonVal->GetString(), "126 dBm");


    // OS flags
    rapidjson::Value* flagsValueJsonVal = rapidjson::Pointer("/data/rsp/osRead/flags/value").Get(responseDoc);
    EXPECT_NE(flagsValueJsonVal, nullptr);
    EXPECT_EQ(flagsValueJsonVal->GetInt(), 5);

    rapidjson::Value* insufficientOsBuildJsonVal = rapidjson::Pointer("/data/rsp/osRead/flags/insufficientOsBuild").Get(responseDoc);
    EXPECT_NE(insufficientOsBuildJsonVal, nullptr);
    EXPECT_EQ(insufficientOsBuildJsonVal->GetBool(), true);

    rapidjson::Value* interfaceTypeJsonVal = rapidjson::Pointer("/data/rsp/osRead/flags/interfaceType").Get(responseDoc);
    EXPECT_NE(interfaceTypeJsonVal, nullptr);
    EXPECT_STREQ(interfaceTypeJsonVal->GetString(), "SPI");

    rapidjson::Value* dpaHandlerDetectedJsonVal = rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerDetected").Get(responseDoc);
    EXPECT_NE(dpaHandlerDetectedJsonVal, nullptr);
    EXPECT_EQ(dpaHandlerDetectedJsonVal->GetBool(), true);

    rapidjson::Value* dpaHandlerNotDetectedButEnabledJsonVal = rapidjson::Pointer("/data/rsp/osRead/flags/dpaHandlerNotDetectedButEnabled").Get(responseDoc);
    EXPECT_NE(dpaHandlerNotDetectedButEnabledJsonVal, nullptr);
    EXPECT_EQ(dpaHandlerNotDetectedButEnabledJsonVal->GetBool(), false);

    rapidjson::Value* noInterfaceSupportedJsonVal = rapidjson::Pointer("/data/rsp/osRead/flags/noInterfaceSupported").Get(responseDoc);
    EXPECT_NE(noInterfaceSupportedJsonVal, nullptr);
    EXPECT_EQ(noInterfaceSupportedJsonVal->GetBool(), false);

    // OS slot limits
    rapidjson::Value* slotLimitsValJsonVal = rapidjson::Pointer("/data/rsp/osRead/slotLimits/value").Get(responseDoc);
    EXPECT_NE(slotLimitsValJsonVal, nullptr);
    EXPECT_EQ(slotLimitsValJsonVal->GetInt(), 49);

    rapidjson::Value* shortestTimeslotJsonVal = rapidjson::Pointer("/data/rsp/osRead/slotLimits/shortestTimeslot").Get(responseDoc);
    EXPECT_NE(shortestTimeslotJsonVal, nullptr);
    EXPECT_STREQ(shortestTimeslotJsonVal->GetString(), "40 ms");

    rapidjson::Value* longestTimeslotJsonVal = rapidjson::Pointer("/data/rsp/osRead/slotLimits/longestTimeslot").Get(responseDoc);
    EXPECT_NE(longestTimeslotJsonVal, nullptr);
    EXPECT_STREQ(longestTimeslotJsonVal->GetString(), "60 ms");


    // peripheral enumeration
    rapidjson::Value* dpaVerJsonVal = rapidjson::Pointer("/data/rsp/peripheralEnumeration/dpaVer").Get(responseDoc);
    EXPECT_NE(dpaVerJsonVal, nullptr);
    EXPECT_STREQ(dpaVerJsonVal->GetString(), "3.2");

    rapidjson::Value* perNrJsonVal = rapidjson::Pointer("/data/rsp/peripheralEnumeration/perNr").Get(responseDoc);
    EXPECT_NE(perNrJsonVal, nullptr);
    EXPECT_EQ(perNrJsonVal->GetInt(), 0);

    // embedded peripheral presence - byte values
    rapidjson::Value* embPersJsonVal = rapidjson::Pointer("/data/rsp/peripheralEnumeration/embPers").Get(responseDoc);
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

    rapidjson::Value* hwpIdJsonVal = rapidjson::Pointer("/data/rsp/peripheralEnumeration/hwpId").Get(responseDoc);
    EXPECT_NE(hwpIdJsonVal, nullptr);
    EXPECT_STREQ(hwpIdJsonVal->GetString(), "0000");
    
    rapidjson::Value* hwpIdVerJsonVal = rapidjson::Pointer("/data/rsp/peripheralEnumeration/hwpIdVer").Get(responseDoc);
    EXPECT_NE(hwpIdVerJsonVal, nullptr);
    EXPECT_EQ(hwpIdVerJsonVal->GetInt(), 0);

    // peripheral enumeration->flags
    rapidjson::Value* perEnumValueJsonVal = rapidjson::Pointer("/data/rsp/peripheralEnumeration/flags/value").Get(responseDoc);
    EXPECT_NE(perEnumValueJsonVal, nullptr);
    EXPECT_EQ(perEnumValueJsonVal->GetInt(), 1);

    rapidjson::Value* rfModeJsonVal = rapidjson::Pointer("/data/rsp/peripheralEnumeration/flags/rfMode").Get(responseDoc);
    EXPECT_NE(rfModeJsonVal, nullptr);
    EXPECT_STREQ(rfModeJsonVal->GetString(), "std");
    
    // user pers - byte values
    rapidjson::Value* userPersJsonVal = rapidjson::Pointer("/data/rsp/peripheralEnumeration/userPers").Get(responseDoc);
    EXPECT_NE(userPersJsonVal, nullptr);

    if (!HasNonfatalFailure()) {
      EXPECT_EQ(userPersJsonVal->IsArray(), true);
      EXPECT_EQ(userPersJsonVal->Size(), 12);

      // parsing and checking raw object
      EXPECT_EQ((*userPersJsonVal)[0].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[1].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[2].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[3].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[4].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[5].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[6].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[7].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[8].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[9].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[10].GetInt(), 0);
      EXPECT_EQ((*userPersJsonVal)[11].GetInt(), 0);
    }

    // TR configuration
    // embedded peripheral presence - byte values
    rapidjson::Value* trConfigEmbPersJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/values").Get(responseDoc);
    EXPECT_NE(trConfigEmbPersJsonVal, nullptr);

    if (!HasNonfatalFailure()) {
      EXPECT_EQ(trConfigEmbPersJsonVal->IsArray(), true);
      EXPECT_EQ(trConfigEmbPersJsonVal->Size(), 4);

      // parsing and checking raw object
      EXPECT_EQ((*trConfigEmbPersJsonVal)[0].GetInt(), 253);
      EXPECT_EQ((*trConfigEmbPersJsonVal)[1].GetInt(), 36);
      EXPECT_EQ((*trConfigEmbPersJsonVal)[2].GetInt(), 0);
      EXPECT_EQ((*trConfigEmbPersJsonVal)[3].GetInt(), 0);
    }

    // embedded peripheral presence - parsed
    rapidjson::Value* coordJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/coordinator").Get(responseDoc);
    EXPECT_NE(coordJsonVal, nullptr);
    EXPECT_EQ(coordJsonVal->GetBool(), true);

    rapidjson::Value* nodeJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/node").Get(responseDoc);
    EXPECT_NE(nodeJsonVal, nullptr);
    EXPECT_EQ(nodeJsonVal->GetBool(), false);

    rapidjson::Value* osJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/os").Get(responseDoc);
    EXPECT_NE(osJsonVal, nullptr);
    EXPECT_EQ(osJsonVal->GetBool(), true);

    rapidjson::Value* eepromJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/eeprom").Get(responseDoc);
    EXPECT_NE(eepromJsonVal, nullptr);
    EXPECT_EQ(eepromJsonVal->GetBool(), true);

    rapidjson::Value* eeepromJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/eeeprom").Get(responseDoc);
    EXPECT_NE(eeepromJsonVal, nullptr);
    EXPECT_EQ(eeepromJsonVal->GetBool(), true);

    rapidjson::Value* ramJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/ram").Get(responseDoc);
    EXPECT_NE(ramJsonVal, nullptr);
    EXPECT_EQ(ramJsonVal->GetBool(), true);

    rapidjson::Value* ledrJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/ledr").Get(responseDoc);
    EXPECT_NE(ledrJsonVal, nullptr);
    EXPECT_EQ(ledrJsonVal->GetBool(), true);

    rapidjson::Value* ledgJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/ledg").Get(responseDoc);
    EXPECT_NE(ledgJsonVal, nullptr);
    EXPECT_EQ(ledgJsonVal->GetBool(), true);

    rapidjson::Value* spiJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/spi").Get(responseDoc);
    EXPECT_NE(spiJsonVal, nullptr);
    EXPECT_EQ(spiJsonVal->GetBool(), false);

    rapidjson::Value* ioJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/io").Get(responseDoc);
    EXPECT_NE(ioJsonVal, nullptr);
    EXPECT_EQ(ioJsonVal->GetBool(), false);

    rapidjson::Value* thermometerJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/thermometer").Get(responseDoc);
    EXPECT_NE(thermometerJsonVal, nullptr);
    EXPECT_EQ(thermometerJsonVal->GetBool(), true);

    rapidjson::Value* pwmJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/pwm").Get(responseDoc);
    EXPECT_NE(pwmJsonVal, nullptr);
    EXPECT_EQ(pwmJsonVal->GetBool(), false);

    rapidjson::Value* uartJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/uart").Get(responseDoc);
    EXPECT_NE(uartJsonVal, nullptr);
    EXPECT_EQ(uartJsonVal->GetBool(), false);

    rapidjson::Value* frcJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/embPers/frc").Get(responseDoc);
    EXPECT_NE(frcJsonVal, nullptr);
    EXPECT_EQ(frcJsonVal->GetBool(), true);


    // other fields read from HWP configuration block
    rapidjson::Value* customDpaHandlerJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/customDpaHandler").Get(responseDoc);
    EXPECT_NE(customDpaHandlerJsonVal, nullptr);
    EXPECT_EQ(customDpaHandlerJsonVal->GetBool(), false);

    rapidjson::Value* nodeDpaInterfaceJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/nodeDpaInterface").Get(responseDoc);
    EXPECT_NE(nodeDpaInterfaceJsonVal, nullptr);
    EXPECT_EQ(nodeDpaInterfaceJsonVal->GetBool(), false);

    rapidjson::Value* dpaAutoexecJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/dpaAutoexec").Get(responseDoc);
    EXPECT_NE(dpaAutoexecJsonVal, nullptr);
    EXPECT_EQ(dpaAutoexecJsonVal->GetBool(), false);

    rapidjson::Value* routingOffJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/routingOff").Get(responseDoc);
    EXPECT_NE(routingOffJsonVal, nullptr);
    EXPECT_EQ(routingOffJsonVal->GetBool(), false);

    rapidjson::Value* ioSetupJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/ioSetup").Get(responseDoc);
    EXPECT_NE(ioSetupJsonVal, nullptr);
    EXPECT_EQ(ioSetupJsonVal->GetBool(), false);

    rapidjson::Value* peerToPeerJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/peerToPeer").Get(responseDoc);
    EXPECT_NE(peerToPeerJsonVal, nullptr);
    EXPECT_EQ(peerToPeerJsonVal->GetBool(), false);

    rapidjson::Value* rfBandJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfBand").Get(responseDoc);
    EXPECT_NE(rfBandJsonVal, nullptr);
    EXPECT_STREQ(rfBandJsonVal->GetString(), "868");

    rapidjson::Value* rfChannelAJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfChannelA").Get(responseDoc);
    EXPECT_NE(rfChannelAJsonVal, nullptr);
    EXPECT_EQ(rfChannelAJsonVal->GetInt(), 33);

    rapidjson::Value* rfChannelBJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfChannelB").Get(responseDoc);
    EXPECT_NE(rfChannelBJsonVal, nullptr);
    EXPECT_EQ(rfChannelBJsonVal->GetInt(), 41);

    rapidjson::Value* rfSubChannelAJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfSubChannelA").Get(responseDoc);
    EXPECT_NE(rfSubChannelAJsonVal, nullptr);
    EXPECT_EQ(rfSubChannelAJsonVal->GetInt(), 8);

    rapidjson::Value* rfSubChannelBJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfSubChannelB").Get(responseDoc);
    EXPECT_NE(rfSubChannelBJsonVal, nullptr);
    EXPECT_EQ(rfSubChannelBJsonVal->GetInt(), 0);

    rapidjson::Value* txPowerJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/txPower").Get(responseDoc);
    EXPECT_NE(txPowerJsonVal, nullptr);
    EXPECT_EQ(txPowerJsonVal->GetInt(), 6);

    rapidjson::Value* rxFilterJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rxFilter").Get(responseDoc);
    EXPECT_NE(rxFilterJsonVal, nullptr);
    EXPECT_EQ(rxFilterJsonVal->GetInt(), 6);

    rapidjson::Value* lpRxTimeoutJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/lpRxTimeout").Get(responseDoc);
    EXPECT_NE(lpRxTimeoutJsonVal, nullptr);
    EXPECT_EQ(lpRxTimeoutJsonVal->GetInt(), 6);

    rapidjson::Value* rfPgmAltChannelJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfPgmAltChannel").Get(responseDoc);
    EXPECT_NE(rfPgmAltChannelJsonVal, nullptr);
    EXPECT_EQ(rfPgmAltChannelJsonVal->GetInt(), 0);

    rapidjson::Value* uartBaudrateJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/uartBaudrate").Get(responseDoc);
    EXPECT_NE(uartBaudrateJsonVal, nullptr);
    EXPECT_EQ(uartBaudrateJsonVal->GetInt(), 9600);


    // RFPGM subfields
    rapidjson::Value* rfPgmDualChannelJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfPgmDualChannel").Get(responseDoc);
    EXPECT_NE(rfPgmDualChannelJsonVal, nullptr);
    EXPECT_EQ(rfPgmDualChannelJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmLpModeJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfPgmLpMode").Get(responseDoc);
    EXPECT_NE(rfPgmLpModeJsonVal, nullptr);
    EXPECT_EQ(rfPgmLpModeJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmIncorrectUploadJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfPgmIncorrectUpload").Get(responseDoc);
    EXPECT_NE(rfPgmIncorrectUploadJsonVal, nullptr);
    EXPECT_EQ(rfPgmIncorrectUploadJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmEnableAfterResetJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfPgmEnableAfterReset").Get(responseDoc);
    EXPECT_NE(rfPgmEnableAfterResetJsonVal, nullptr);
    EXPECT_EQ(rfPgmEnableAfterResetJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmTerminateAfter1MinJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfPgmTerminateAfter1Min").Get(responseDoc);
    EXPECT_NE(rfPgmTerminateAfter1MinJsonVal, nullptr);
    EXPECT_EQ(rfPgmTerminateAfter1MinJsonVal->GetBool(), false);

    rapidjson::Value* rfPgmTerminateMcuPinJsonVal = rapidjson::Pointer("/data/rsp/trConfiguration/rfPgmTerminateMcuPin").Get(responseDoc);
    EXPECT_NE(rfPgmTerminateMcuPinJsonVal, nullptr);
    EXPECT_EQ(rfPgmTerminateMcuPinJsonVal->GetBool(), false);


    // more peripherals info
    rapidjson::Value* morePerInfoJson = rapidjson::Pointer("/data/rsp/morePeripheralsInfo").Get(responseDoc);
    EXPECT_NE(morePerInfoJson, nullptr);

    if (!HasNonfatalFailure()) {
      EXPECT_EQ(morePerInfoJson->IsArray(), true);
      EXPECT_EQ(morePerInfoJson->Size(), 14);

      // parsing and checking More Per Info object
      EXPECT_EQ((*morePerInfoJson)[0].IsObject(), true);

      // OBJECT 1
      // perTe
      rapidjson::Value::MemberIterator perTeIter = (*morePerInfoJson)[0].FindMember("perTe");
      EXPECT_NE(perTeIter, (*morePerInfoJson)[0].MemberEnd());
      EXPECT_EQ(perTeIter->value.GetInt(), 2);

      // perT
      rapidjson::Value::MemberIterator perTIter = (*morePerInfoJson)[0].FindMember("perT");
      EXPECT_NE(perTIter, (*morePerInfoJson)[0].MemberEnd());
      EXPECT_EQ(perTIter->value.GetInt(), 3);

      // par1
      rapidjson::Value::MemberIterator par1Iter = (*morePerInfoJson)[0].FindMember("par1");
      EXPECT_NE(par1Iter, (*morePerInfoJson)[0].MemberEnd());
      EXPECT_EQ(par1Iter->value.GetInt(), 0);

      // par2
      rapidjson::Value::MemberIterator par2Iter = (*morePerInfoJson)[0].FindMember("par2");
      EXPECT_NE(par2Iter, (*morePerInfoJson)[0].MemberEnd());
      EXPECT_EQ(par2Iter->value.GetInt(), 0);


      // OBJECT 2
      // perTe
      perTeIter = (*morePerInfoJson)[1].FindMember("perTe");
      EXPECT_NE(perTeIter, (*morePerInfoJson)[1].MemberEnd());
      EXPECT_EQ(perTeIter->value.GetInt(), 36);

      // perT
      perTIter = (*morePerInfoJson)[1].FindMember("perT");
      EXPECT_NE(perTIter, (*morePerInfoJson)[1].MemberEnd());
      EXPECT_EQ(perTIter->value.GetInt(), 0);

      // par1
      par1Iter = (*morePerInfoJson)[1].FindMember("par1");
      EXPECT_NE(par1Iter, (*morePerInfoJson)[1].MemberEnd());
      EXPECT_EQ(par1Iter->value.GetInt(), 0);

      // par2
      par2Iter = (*morePerInfoJson)[1].FindMember("par2");
      EXPECT_NE(par2Iter, (*morePerInfoJson)[1].MemberEnd());
      EXPECT_EQ(par2Iter->value.GetInt(), 0);


      // NEXT objects up to 14
      for (int i = 2; i < 13; i++) {
        // perTe
        perTeIter = (*morePerInfoJson)[i].FindMember("perTe");
        EXPECT_NE(perTeIter, (*morePerInfoJson)[i].MemberEnd());
        EXPECT_EQ(perTeIter->value.GetInt(), 0);

        // perT
        perTIter = (*morePerInfoJson)[i].FindMember("perT");
        EXPECT_NE(perTIter, (*morePerInfoJson)[i].MemberEnd());
        EXPECT_EQ(perTIter->value.GetInt(), 0);

        // par1
        par1Iter = (*morePerInfoJson)[i].FindMember("par1");
        EXPECT_NE(par1Iter, (*morePerInfoJson)[i].MemberEnd());
        EXPECT_EQ(par1Iter->value.GetInt(), 0);

        // par2
        par2Iter = (*morePerInfoJson)[i].FindMember("par2");
        EXPECT_NE(par2Iter, (*morePerInfoJson)[i].MemberEnd());
        EXPECT_EQ(par2Iter->value.GetInt(), 0);
      }

    }
    

    // raw data
    rapidjson::Value* rawDataJson = rapidjson::Pointer("/data/raw").Get(responseDoc);
    EXPECT_NE(rawDataJson, nullptr);

    if (!HasNonfatalFailure()) {
      EXPECT_EQ(rawDataJson->IsArray(), true);
      EXPECT_EQ(rawDataJson->Size(), 8);

      // parsing and checking raw object
      EXPECT_EQ((*rawDataJson)[0].IsObject(), true);

      // request
      rapidjson::Value::MemberIterator requestIter = (*rawDataJson)[0].FindMember("request");
      EXPECT_NE(requestIter, (*rawDataJson)[0].MemberEnd());
      EXPECT_STREQ(requestIter->value.GetString(), "00.00.00.0a.ff.ff.20.00");

      // confirmation
      rapidjson::Value::MemberIterator confirmIter = (*rawDataJson)[0].FindMember("confirmation");
      EXPECT_NE(confirmIter, (*rawDataJson)[0].MemberEnd());
      EXPECT_STREQ(confirmIter->value.GetString(), "");

      // response
      rapidjson::Value::MemberIterator responseIter = (*rawDataJson)[0].FindMember("response");
      EXPECT_NE(responseIter, (*rawDataJson)[0].MemberEnd());
      EXPECT_STREQ(
        responseIter->value.GetString(), 
        "00.00.00.8a.00.00.00.00.3f.3a.03.1d.0e.32.a1.08.03.1d.0b.32.ef.30.29.00.a7.00.ee.30.a8.00.ef.30.a9.00.ee.30.aa.00.03.14.08.00.03.10.29.00.08.00.00.00.00.00.00.00.00.00"
      );

      // request
      requestIter = (*rawDataJson)[1].FindMember("request");
      EXPECT_NE(requestIter, (*rawDataJson)[1].MemberEnd());
      EXPECT_STREQ(requestIter->value.GetString(), "00.00.00.0a.ff.ff.00.50");

      // confirmation
      confirmIter = (*rawDataJson)[1].FindMember("confirmation");
      EXPECT_NE(confirmIter, (*rawDataJson)[1].MemberEnd());
      EXPECT_STREQ(confirmIter->value.GetString(), "");

      // response
      responseIter = (*rawDataJson)[1].FindMember("response");
      EXPECT_NE(responseIter, (*rawDataJson)[1].MemberEnd());
      EXPECT_STREQ(
        responseIter->value.GetString(),
        "00.00.00.8a.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00"
      );

      // request
      requestIter = (*rawDataJson)[2].FindMember("request");
      EXPECT_NE(requestIter, (*rawDataJson)[2].MemberEnd());
      EXPECT_STREQ(requestIter->value.GetString(), "00.00.00.0a.ff.ff.00.52");

      // confirmation
      confirmIter = (*rawDataJson)[2].FindMember("confirmation");
      EXPECT_NE(confirmIter, (*rawDataJson)[2].MemberEnd());
      EXPECT_STREQ(confirmIter->value.GetString(), "");

      // response
      responseIter = (*rawDataJson)[2].FindMember("response");
      EXPECT_NE(responseIter, (*rawDataJson)[2].MemberEnd());
      EXPECT_STREQ(
        responseIter->value.GetString(),
        "00.00.00.8a.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00"
      );

      // request
      requestIter = (*rawDataJson)[3].FindMember("request");
      EXPECT_NE(requestIter, (*rawDataJson)[3].MemberEnd());
      EXPECT_STREQ(requestIter->value.GetString(), "00.00.00.0a.ff.ff.00.53");

      // confirmation
      confirmIter = (*rawDataJson)[3].FindMember("confirmation");
      EXPECT_NE(confirmIter, (*rawDataJson)[3].MemberEnd());
      EXPECT_STREQ(confirmIter->value.GetString(), "");

      // response
      responseIter = (*rawDataJson)[3].FindMember("response");
      EXPECT_NE(responseIter, (*rawDataJson)[3].MemberEnd());
      EXPECT_STREQ(
        responseIter->value.GetString(),
        "00.00.00.8a.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00"
      );

      // request
      requestIter = (*rawDataJson)[4].FindMember("request");
      EXPECT_NE(requestIter, (*rawDataJson)[4].MemberEnd());
      EXPECT_STREQ(requestIter->value.GetString(), "00.00.02.00.ff.ff");

      // confirmation
      confirmIter = (*rawDataJson)[4].FindMember("confirmation");
      EXPECT_NE(confirmIter, (*rawDataJson)[4].MemberEnd());
      EXPECT_STREQ(confirmIter->value.GetString(), "");

      // response
      responseIter = (*rawDataJson)[4].FindMember("response");
      EXPECT_NE(responseIter, (*rawDataJson)[4].MemberEnd());
      EXPECT_STREQ(
        responseIter->value.GetString(),
        "00.00.02.80.00.00.00.00.dc.9c.00.81.40.24.91.08.00.28.05.31"
      );

      // request
      requestIter = (*rawDataJson)[5].FindMember("request");
      EXPECT_NE(requestIter, (*rawDataJson)[5].MemberEnd());
      EXPECT_STREQ(requestIter->value.GetString(), "00.00.ff.3f.ff.ff");

      // confirmation
      confirmIter = (*rawDataJson)[5].FindMember("confirmation");
      EXPECT_NE(confirmIter, (*rawDataJson)[5].MemberEnd());
      EXPECT_STREQ(confirmIter->value.GetString(), "");

      // response
      responseIter = (*rawDataJson)[5].FindMember("response");
      EXPECT_NE(responseIter, (*rawDataJson)[5].MemberEnd());
      EXPECT_STREQ(
        responseIter->value.GetString(),
        "00.00.ff.bf.00.00.00.00.02.03.00.fd.24.00.00.00.00.00.00.01"
      );

      // request
      requestIter = (*rawDataJson)[6].FindMember("request");
      EXPECT_NE(requestIter, (*rawDataJson)[6].MemberEnd());
      EXPECT_STREQ(requestIter->value.GetString(), "00.00.02.02.ff.ff");

      // confirmation
      confirmIter = (*rawDataJson)[6].FindMember("confirmation");
      EXPECT_NE(confirmIter, (*rawDataJson)[6].MemberEnd());
      EXPECT_STREQ(confirmIter->value.GetString(), "");

      // response
      responseIter = (*rawDataJson)[6].FindMember("response");
      EXPECT_NE(responseIter, (*rawDataJson)[6].MemberEnd());
      EXPECT_STREQ(
        responseIter->value.GetString(),
        "00.00.02.82.00.00.00.00.b4.c9.10.34.34.34.3c.34.32.32.32.37.34.34.34.34.34.15.1d.34.34.34.34.34.34.34.34.34.34.34.37.34.00.30"
      );

      // request
      requestIter = (*rawDataJson)[7].FindMember("request");
      EXPECT_NE(requestIter, (*rawDataJson)[7].MemberEnd());
      EXPECT_STREQ(requestIter->value.GetString(), "00.00.ff.3f.ff.ff");

      // confirmation
      confirmIter = (*rawDataJson)[7].FindMember("confirmation");
      EXPECT_NE(confirmIter, (*rawDataJson)[7].MemberEnd());
      EXPECT_STREQ(confirmIter->value.GetString(), "");

      // response
      responseIter = (*rawDataJson)[7].FindMember("response");
      EXPECT_NE(responseIter, (*rawDataJson)[7].MemberEnd());
      EXPECT_STREQ(
        responseIter->value.GetString(),
        "00.00.ff.bf.00.00.00.00.02.03.00.fd.24.00.00.00.00.00.00.01"
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
  TEST_F(DeviceEnumerationTesting, iqmeshNetwork_EnumerateDevice_2)
  {
    // JSON request - as received from messaging
    std::string requestStr =
      "{"
      "  \"mType\": \"iqmeshNetwork_EnumerateDevice\","
      "  \"data\" : {"
      "    \"msgId\": \"testEnumerateDevice\","
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
    // Enumeration Device Service should send error JSON response WITHOUT ever sending some DPA request

    // expected JSON message output (jmo) as result of processing to be sent out by a messaging
    std::string responseStr = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);

    rapidjson::Document responseDoc;
    responseDoc.Parse(responseStr);


    // testing response data field values
    rapidjson::Value* msgMtypeJsonVal = rapidjson::Pointer("/mType").Get(responseDoc);
    EXPECT_NE(msgMtypeJsonVal, nullptr);
    EXPECT_STREQ(msgMtypeJsonVal->GetString(), "iqmeshNetwork_EnumerateDevice");

    rapidjson::Value* msgIdJsonVal = rapidjson::Pointer("/data/msgId").Get(responseDoc);
    EXPECT_NE(msgIdJsonVal, nullptr);
    EXPECT_STREQ(msgIdJsonVal->GetString(), "testEnumerateDevice");

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
  TEST_F(DeviceEnumerationTesting, iqmeshNetwork_EnumerateDevice_3)
  {
    // JSON request - as received from messaging
    std::string requestStr =
      "{"
      "  \"mType\": \"iqmeshNetwork_EnumerateDevice\","
      "  \"data\" : {"
      "    \"msgId\": \"testEnumerateDevice\","
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
    // Enumearte Device Service should send error JSON response WITHOUT ever sending some DPA request

    // expected JSON message output (jmo) as result of processing to be sent out by a messaging
    std::string responseStr = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);

    rapidjson::Document responseDoc;
    responseDoc.Parse(responseStr);


    // testing response data field values
    rapidjson::Value* msgMtypeJsonVal = rapidjson::Pointer("/mType").Get(responseDoc);
    EXPECT_NE(msgMtypeJsonVal, nullptr);
    EXPECT_STREQ(msgMtypeJsonVal->GetString(), "iqmeshNetwork_EnumerateDevice");

    rapidjson::Value* msgIdJsonVal = rapidjson::Pointer("/data/msgId").Get(responseDoc);
    EXPECT_NE(msgIdJsonVal, nullptr);
    EXPECT_STREQ(msgIdJsonVal->GetString(), "testEnumerateDevice");

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
