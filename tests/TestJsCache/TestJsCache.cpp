#include "ITestSimulationIRestApiService.h"
#include "TestJsCache.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "HexStringCoversion.h"

#include "gtest/gtest.h"

#include "rapidjson/pointer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"

#include "Urls.h"

#include "iqrf__TestJsCache.hxx"

TRC_INIT_MNAME(iqrf::TestJsCache)

namespace iqrf {

  class Imp {
  private:
    Imp()
    {
    }

  public:
    iqrf::IJsRenderService* m_iJsRenderService = nullptr;
    iqrf::IJsCacheService* m_iJsCacheService = nullptr;
    shape::ILaunchService* m_iLaunchService = nullptr;
    shape::IConfigurationService* m_iConfigurationService = nullptr;
    shape::ITestSimulationIRestApiService* m_iTestSimulationIRestApiService = nullptr;

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
        "TestJsCache instance activate" << std::endl <<
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
        "TestJsCache instance deactivate" << std::endl <<
        "******************************"
      );
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

    void attachInterface(iqrf::IJsCacheService* iface)
    {
      m_iJsCacheService = iface;
    }

    void detachInterface(iqrf::IJsCacheService* iface)
    {
      if (m_iJsCacheService == iface) {
        m_iJsCacheService = nullptr;
      }
    }

    void attachInterface(shape::ITestSimulationIRestApiService* iface)
    {
      m_iTestSimulationIRestApiService = iface;
    }

    void detachInterface(shape::ITestSimulationIRestApiService* iface)
    {
      if (m_iTestSimulationIRestApiService == iface) {
        m_iTestSimulationIRestApiService = nullptr;
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

    void attachInterface(shape::IConfigurationService* iface)
    {
      m_iConfigurationService = iface;
    }

    void detachInterface(shape::IConfigurationService* iface)
    {
      if (m_iConfigurationService == iface) {
        m_iConfigurationService = nullptr;
      }
    }

  };

  ////////////////////////////////////
  TestJsCache::TestJsCache()
  {
  }

  TestJsCache::~TestJsCache()
  {
  }

  void TestJsCache::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestJsCache::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestJsCache::modify(const shape::Properties *props)
  {
  }

  void TestJsCache::attachInterface(iqrf::IJsRenderService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsCache::detachInterface(iqrf::IJsRenderService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsCache::attachInterface(iqrf::IJsCacheService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsCache::detachInterface(iqrf::IJsCacheService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsCache::attachInterface(shape::ITestSimulationIRestApiService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsCache::detachInterface(shape::ITestSimulationIRestApiService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsCache::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsCache::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsCache::attachInterface(shape::IConfigurationService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestJsCache::detachInterface(shape::IConfigurationService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestJsCache::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestJsCache::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class JsCacheTesting : public ::testing::Test
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

    static bool copyTestData(const std::string& srcName, const std::string& dstName)
    {
      std::ifstream  src(srcName, std::ios::binary);
      if (!src.is_open())
        return false;
      std::ofstream  dst(dstName, std::ios::binary);
      if (!dst.is_open())
        return false;
      dst << src.rdbuf();
      src.close();
      dst.close();
      return true;
    }

    static bool prepareTestData()
    {
      //prepare data for the test as it was changed by previous run of this test
      bool res = true;

      //copy original server
      res = copyTestData(
        "./configuration/testData/server/org_data.json",
        "./configuration/testRepoCache/iqrfRepoCache2/server/data.json");
      if (!res)
        return res;

      //copy updated server
      res = copyTestData(
        "./configuration/testData/server/upd_data.json",
        "./configuration/testResources/iqrfRepoResource2/api/server/data.json");
      if (!res)
        return res;

      //copy original osdpa
      res = copyTestData(
        "./configuration/testData/osdpa/org_data.json",
        "./configuration/testRepoCache/iqrfRepoCache2/osdpa/data.json");
      if (!res)
        return res;

      //copy updated osdpa
      res = copyTestData(
        "./configuration/testData/osdpa/upd_data.json",
        "./configuration/testResources/iqrfRepoResource2/api/osdpa/data.json");
      if (!res)
        return res;

      //copy original standards/10/0
      res = copyTestData(
        "./configuration/testData/standards/org_data_10_0.json",
        "./configuration/testRepoCache/iqrfRepoCache2/standards/10/0/data.json");
      if (!res)
        return res;

      //copy updated standards/10/0
      res = copyTestData(
        "./configuration/testData/standards/upd_data_10_0.json",
        "./configuration/testResources/iqrfRepoResource2/api/standards/10/0/data.json");
      return res;

    }
  };

  /////////// Tests
  //TODO missing:

  const unsigned MILLIS_WAIT = 2000;
  const int64_t ORIGINAL_SUM = 3555947887021610939;
  const int64_t UPDATED_SUM = 4055947887021610939;

  TEST_F(JsCacheTesting, GetServerState1)
  {
    auto  serverState = Imp::get().m_iJsCacheService->getServerState();
    int64_t sum = serverState.m_databaseChecksum;
    EXPECT_EQ(ORIGINAL_SUM, sum);
  }

  TEST_F(JsCacheTesting, GetOsDpa1)
  {
    auto  o = Imp::get().m_iJsCacheService->getOsDpa(3);
    ASSERT_FALSE(o == nullptr);
    EXPECT_EQ("08BF", o->m_os);
    EXPECT_EQ("0303", o->m_dpa);
    o = Imp::get().m_iJsCacheService->getOsDpa(4);
    ASSERT_TRUE(o == nullptr);
  }

  TEST_F(JsCacheTesting, CallJsScript1)
  {
    std::string functionName = "iqrf.test.cache.convert";
    std::string par = "\"asdfgh\"";
    std::string ret;
    try {
      Imp::get().m_iJsRenderService->call(functionName, par, ret);
    }
    catch (std::exception &e) {
      EXPECT_EQ("\"asdfgh\"", par);
    }
  }

  TEST_F(JsCacheTesting, ChangeCacheConfig)
  {
    using namespace rapidjson;

    shape::IConfiguration* cfg = Imp::get().m_iConfigurationService->getConfiguration("iqrf::JsCache", "JsCache");
    ASSERT_FALSE(cfg == nullptr);
    auto props = cfg->getProperties();
    ASSERT_FALSE(props == nullptr);

    std::string urlRepo, iqrfRepoCache;
    double checkPeriodInMinutes = -1;

    Document& doc = props->getAsJson();

    auto v1 = Pointer("/checkPeriodInMinutes").Get(doc);
    ASSERT_TRUE(v1 != nullptr && v1->IsNumber());
    checkPeriodInMinutes = v1->GetDouble();

    auto v2 = Pointer("/urlRepo").Get(doc);
    ASSERT_TRUE(v2 != nullptr && v2->IsString());
    urlRepo = v2->GetString();

    auto v3 = Pointer("/iqrfRepoCache").Get(doc);
    ASSERT_TRUE(v3 != nullptr && v3->IsString());
    iqrfRepoCache = v3->GetString();

    //no repo check up to now
    EXPECT_EQ(0, checkPeriodInMinutes);
    EXPECT_EQ("https://repository.iqrfalliance.org/api", urlRepo);
    ASSERT_EQ("testRepoCache/iqrfRepoCache1", iqrfRepoCache);

    JsCacheTesting::prepareTestData();

    Imp::get().m_iTestSimulationIRestApiService->setResourceDirectory("iqrfRepoResource2");

    //update cfg to repo check every 0.02 min
    Pointer("/checkPeriodInMinutes").Set(doc, 0.02);
    Pointer("/iqrfRepoCache").Set(doc, "testRepoCache/iqrfRepoCache2");
    cfg->update();

    for (const char* url : testUrls) {
      std::string inReq = Imp::get().m_iTestSimulationIRestApiService->popIncomingRequest(MILLIS_WAIT);
      EXPECT_EQ(url, inReq);
    }

    auto  serverState = Imp::get().m_iJsCacheService->getServerState();
    int64_t sum = serverState.m_databaseChecksum;
    EXPECT_EQ(UPDATED_SUM, sum);

  }
  
  TEST_F(JsCacheTesting, GetServerState2)
  {
    auto  serverState = Imp::get().m_iJsCacheService->getServerState();
    int64_t sum = serverState.m_databaseChecksum;
    EXPECT_EQ(UPDATED_SUM, sum);
  }

  TEST_F(JsCacheTesting, GetOsDpa2)
  {
    auto  o = Imp::get().m_iJsCacheService->getOsDpa(3);
    ASSERT_FALSE(o == nullptr);
    EXPECT_EQ("08BF", o->m_os);
    EXPECT_EQ("0303", o->m_dpa);
    o = Imp::get().m_iJsCacheService->getOsDpa(4);
    ASSERT_FALSE(o == nullptr);
    EXPECT_EQ("TEST", o->m_os);
    EXPECT_EQ("0001", o->m_dpa);
  }

  TEST_F(JsCacheTesting, CallJsScript2)
  {
    std::string functionName = "iqrf.test.cache.convert";
    std::string par = "\"asdfgh\"";
    std::string ret;
    try {
      Imp::get().m_iJsRenderService->call(functionName, par, ret);
    }
    catch (std::exception &e) {
      EXPECT_EQ("\"asdfgh\"", std::string(e.what()));
    }
    par = ret;
    EXPECT_EQ("\"ASDFGH\"", ret);
  }

}
