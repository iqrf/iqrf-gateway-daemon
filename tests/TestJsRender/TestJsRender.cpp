#include "TestJsRender.h"
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

#include "iqrf__TestJsRender.hxx"

TRC_INIT_MNAME(cobalt::CmCobaltTestTrj)

using namespace std;

namespace iqrf {

  class Imp {
  private:
    Imp()
    {
    }

  public:
    shape::ILaunchService* m_iLaunchService = nullptr;
    iqrf::IJsRenderService* m_iJsRenderService = nullptr;
    std::thread m_thread;
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

      m_gtest.runAllTests(m_iLaunchService);

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("")
    }

    void attachInterface(iqrf::IJsRenderService* iface)
    {
      m_iJsRenderService = iface;
    }

    void detachInterface(iqrf::IJsRenderService* iface)
    {
      if (m_iJsRenderService == iface) {
        m_iJsRenderService = nullptr;
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
  TestJsRender::TestJsRender()
  {
  }

  TestJsRender::~TestJsRender()
  {
  }

  void TestJsRender::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestJsRender::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestJsRender::modify(const shape::Properties *props)
  {
  }

  void TestJsRender::attachInterface(iqrf::IJsRenderService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsRender::detachInterface(iqrf::IJsRenderService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsRender::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsRender::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsRender::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestJsRender::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class JsRenderTesting : public ::testing::Test
  {
  protected:

    void SetUp(void) override
    {
      ASSERT_NE(nullptr, &Imp::get().m_iLaunchService);
      ASSERT_NE(nullptr, &Imp::get().m_iJsRenderService);
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
  TEST_F(JsRenderTesting, loadJsCode)
  {
    std::ifstream jsFile("./TestJavaScript/test.js");
    ASSERT_TRUE(jsFile.is_open());
    std::ostringstream strStream;
    strStream << jsFile.rdbuf();
    std::string jsString = strStream.str();
    ASSERT_FALSE(jsString.empty());
    Imp::get().m_iJsRenderService->loadJsCode(jsString);
  }
  TEST_F(JsRenderTesting, callFunction)
  {
    std::string input = "\"qwerty\"";
    std::string output;
    std::string expect = "{\"out\":\"QWERTY\"}";
    Imp::get().m_iJsRenderService->call("test.convertUpperCase", input, output);
    ASSERT_EQ(expect, output);
  }

#if 0

  TEST_F(JsRenderTesting, loadJsCode)
  {
    std::ifstream jsFile("./TestJavaScript/test.js");
    ASSERT_TRUE(jsFile.is_open());
    std::ostringstream strStream;
    strStream << jsFile.rdbuf();
    std::string jsString = strStream.str();
    ASSERT_FALSE(jsString.empty());
    Imp::get().m_iJsRenderService->loadJsCode(jsString);
  }

  TEST_F(JsRenderTesting, callFunction)
  {
    std::string input = "\"qwerty\"";
    std::string output;
    std::string expect = "{\"out\":\"QWERTY\"}";
    Imp::get().m_iJsRenderService->call("test.convertUpperCase", input, output);
    ASSERT_EQ(expect, output);
  }
}
