#define IMessagingSplitterService_EXPORTS
#define IIqrfChannelService_EXPORTS

#include "TestJsonDpaApiIqrfStandard.h"
#include "IIqrfDpaService.h"
#include "AccessControl.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "HexStringCoversion.h"

#include "gtest/gtest.h"

#include "rapidjson/pointer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <condition_variable>
#include <algorithm>

#include "iqrf__TestJsonDpaApiIqrfStandard.hxx"

TRC_INIT_MNAME(iqrf::TestJsonDpaApiIqrfStandard)

using namespace std;

namespace iqrf {

  const std::string ATTACH_IqrfDpa("ATTACH IqrfDpa");
  
  //aux class to convert from dot notation to ustring and back
  class DotMsg
  {
  public:
    DotMsg(std::basic_string<unsigned char> msg)
      :m_msg(msg)
    {}

    DotMsg(std::string dotMsg)
    {
      if (!dotMsg.empty()) {
        std::string buf = dotMsg;
        std::replace(buf.begin(), buf.end(), '.', ' ');

        std::istringstream istr(buf);

        int val;
        while (true) {
          if (!(istr >> std::hex >> val)) {
            if (istr.eof()) break;
            THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format: " << PAR(dotMsg));
          }
          m_msg.push_back((uint8_t)val);
        }
      }
    }

    operator std::basic_string<unsigned char>() { return m_msg; }

    operator std::string()
    {
      std::string to;
      if (!m_msg.empty()) {
        std::ostringstream ostr;
        ostr.setf(std::ios::hex, std::ios::basefield);
        ostr.fill('0');
        long i = 0;
        for (uint8_t c : m_msg) {
          ostr << std::setw(2) << (short int)c;
          ostr << '.';
        }
        to = ostr.str();
        to.pop_back();
      }
      return to;
    }

  private:
    std::basic_string<unsigned char> m_msg;
  };

  /////////////////////////
  class Imp {
  private:
    Imp()
      :m_accessControl(this)
    {
    }

  public:
    shape::ILaunchService* m_iLaunchService = nullptr;
    iqrf::IIqrfDpaService* m_iIqrfDpaService = nullptr;
    iqrf::ITestSimulationMessaging* m_iTestSimulationMessaging = nullptr;

    std::mutex m_mtx;
    std::condition_variable m_cv;
    std::string m_expectedMessage;

    std::thread m_thread;
    shape::GTestStaticRunner m_gtest;

    AccessControl<Imp> m_accessControl;

    static Imp& get()
    {
      static Imp imp;
      return imp;
    }

    ~Imp()
    {
    }

    ////iqrf::IMessagingSplitterService
    ///////////////////////////////////////
    //void sendMessage(const std::string& messagingId, rapidjson::Document* doc) const
    //{
    //  TRC_FUNCTION_ENTER("");
    //  TRC_FUNCTION_LEAVE("")
    //}

    //void registerFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters, IMessagingSplitterService::FilteredMessageHandlerFunc handlerFunc)
    //{
    //  TRC_FUNCTION_ENTER("");
    //  m_msgTypeFilters = msgTypeFilters;
    //  m_handlerFunc = handlerFunc;
    //  TRC_FUNCTION_LEAVE("")
    //}

    //void unregisterFilteredMsgHandler(const std::vector<std::string>& msgTypeFilters)
    //{
    //  TRC_FUNCTION_ENTER("");
    //  m_msgTypeFilters.clear();
    //  m_handlerFunc = nullptr;
    //  TRC_FUNCTION_LEAVE("")
    //}

    //iqrf::IIqrfChannelService
    /////////////////////////////////////
    IIqrfChannelService::State getState() const
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("")
      return IIqrfChannelService::State::Ready;
    }

    std::unique_ptr<IIqrfChannelService::Accessor> getAccess(IIqrfChannelService::ReceiveFromFunc receiveFromFunc, IIqrfChannelService::AccesType access)
    {
      auto retval = m_accessControl.getAccess(receiveFromFunc, access);
      
      //IqrfDpa has registered - fire test
      m_cv.notify_all();

      return retval;
    }

    bool hasExclusiveAccess() const
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("")
      return false;
    }

    //Accessor
    ///////////////////////////////////
    void send(const std::basic_string<unsigned char>& message)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION("Sending to simulation: " << std::endl << MEM_HEX(message.data(), message.size()));

      std::unique_lock<std::mutex> lck(m_mtx);
      m_expectedMessage = DotMsg(message);
      //std::cout << m_expectedMessage << std::endl;
      m_cv.notify_all();

      TRC_FUNCTION_LEAVE("");
    }

    bool enterProgrammingState()
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return true;
    }

    IIqrfChannelService::Accessor::UploadErrorCode
      upload(
        const IIqrfChannelService::Accessor::UploadTarget target,
        const std::basic_string<uint8_t>& data,
        const uint16_t address
      )
    {
      // write data to TR module
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;
    }

    bool terminateProgrammingState() {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("");
      return true;
    }

    std::string fetchMessage(unsigned millisToWait)
    {
      TRC_FUNCTION_ENTER(PAR(millisToWait));

      std::unique_lock<std::mutex> lck(m_mtx);
      if (m_expectedMessage.empty()) {
        while (m_cv.wait_for(lck, std::chrono::milliseconds(millisToWait)) != std::cv_status::timeout) {
          if (!m_expectedMessage.empty()) break;
        }
      }
      std::string expectedMessage = m_expectedMessage;
      m_expectedMessage.clear();
      TRC_FUNCTION_LEAVE(PAR(expectedMessage));
      return expectedMessage;
    }


    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "TestJsonDpaApiIqrfStandard instance activate" << std::endl <<
        "******************************"
      );

      m_thread = std::thread([&]()
      {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_cv.wait(lck);
        m_gtest.runAllTests(m_iLaunchService);
      });

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

    void attachInterface(iqrf::IIqrfDpaService* iface)
    {
      m_iIqrfDpaService = iface;

      std::unique_lock<std::mutex> lck(m_mtx);
      m_expectedMessage = ATTACH_IqrfDpa;
      m_cv.notify_all();

    }

    void detachInterface(iqrf::IIqrfDpaService* iface)
    {
      if (m_iIqrfDpaService == iface) {
        m_iIqrfDpaService = nullptr;
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

  //iqrf::IIqrfChannelService
  IIqrfChannelService::State TestJsonDpaApiIqrfStandard::getState() const
  {
    return Imp::get().getState();
  }

  std::unique_ptr<IIqrfChannelService::Accessor> TestJsonDpaApiIqrfStandard::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
  {
    return Imp::get().getAccess(receiveFromFunc, access);
  }

  bool TestJsonDpaApiIqrfStandard::hasExclusiveAccess() const
  {
    return Imp::get().hasExclusiveAccess();
  }

  /////////////////////////
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
  }

  void TestJsonDpaApiIqrfStandard::attachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonDpaApiIqrfStandard::detachInterface(iqrf::ITestSimulationMessaging* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsonDpaApiIqrfStandard::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsonDpaApiIqrfStandard::detachInterface(iqrf::IIqrfDpaService* iface)
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
  //parametric tests shall be called for all msgs from .../iqrf-daemon/tests/test-api-app/test-api-app/log/

  const unsigned MILLIS_WAIT = 1000;

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

  TEST_F(JsonDpaApiIqrfStandardTesting, iqrfEmbedCoordinator_AddrInfo)
  {
    std::string imsg =
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

    Imp::get().m_iTestSimulationMessaging->pushIncomingMessage(imsg);
    std::string omsg = Imp::get().m_iTestSimulationMessaging->popOutgoingMessage(1000);
  }

  //TEST_F(JsonDpaApiIqrfStandardTesting, filterRegistration)
  //{
  //  auto & filters = Imp::get().m_msgTypeFilters;
  //  ASSERT_EQ(4, filters.size());

  //  EXPECT_EQ("iqrfEmbed", filters[0]);
  //  EXPECT_EQ("iqrfLight", filters[1]);
  //  EXPECT_EQ("iqrfSensor", filters[2]);
  //  EXPECT_EQ("iqrfBinaryoutput", filters[3]);

  //  ASSERT_NE(nullptr, Imp::get().m_handlerFunc);
  //}

}
