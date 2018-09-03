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

  TEST_F(SchedulerTesting, scheduleTask)
  {
    using namespace rapidjson;
    Document doc1;
    Pointer("/item").Set(doc1, VAL1);
    Document doc2;
    Pointer("/item").Set(doc2, VAL2);

    //schedule two tasks by cron time
    ISchedulerService::TaskHandle th1 = m_iJsRenderService->scheduleTask(CLIENT_ID, doc1, CRON1);
    ISchedulerService::TaskHandle th2 = m_iJsRenderService->scheduleTask(CLIENT_ID, doc2, CRON2);

    //expected two handlers
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iJsRenderService->getMyTasks(CLIENT_ID);
    ASSERT_EQ(2, taskHandleVect.size());
    //tasks ordered according scheduled time so no exact ordering expected
    EXPECT_TRUE(th1 == taskHandleVect[0] || th1 == taskHandleVect[1]);
    EXPECT_TRUE(th2 == taskHandleVect[0] || th2 == taskHandleVect[1]);

    //verify returned task1
    const rapidjson::Value *task1 = m_iJsRenderService->getMyTask(CLIENT_ID, th1);
    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    //verify returned task2
    const rapidjson::Value *task2 = m_iJsRenderService->getMyTask(CLIENT_ID, th2);
    const rapidjson::Value *val2 = Pointer("/item").Get(*task2);
    ASSERT_NE(nullptr, val2);
    ASSERT_TRUE(val2->IsInt());
    EXPECT_EQ(VAL2, val2->GetInt());

    //verify returned task1 timeSpec
    const rapidjson::Value *timeSpec = m_iJsRenderService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, CRON1, false, false, 0, "");

    //remove tasks by vector
    std::vector<ISchedulerService::TaskHandle> taskHandleVectRem = { th1, th2 };
    m_iJsRenderService->removeTasks(CLIENT_ID, taskHandleVectRem);
    
    //verify removal
    taskHandleVect = m_iJsRenderService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }

  TEST_F(SchedulerTesting, scheduleTaskAt)
  {
    using namespace rapidjson;
    using namespace chrono;

    Document doc1;
    Pointer("/item").Set(doc1, VAL1);

    //prepare expiration time
    system_clock::time_point tp = system_clock::now();
    tp += milliseconds(10000); //expire in 10 sec
    string tpStr = encodeTimestamp(tp);

    //schedule
    ISchedulerService::TaskHandle th1 = m_iJsRenderService->scheduleTaskAt(CLIENT_ID, doc1, tp);
    const rapidjson::Value *task1 = m_iJsRenderService->getMyTask(CLIENT_ID, th1);

    //verify returned task1
    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    //verify returned task1 timeSpec
    const rapidjson::Value *timeSpec = m_iJsRenderService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, "", true, false, 0, tpStr); //expiration time as str in tpStr

    //remove tasks by id
    m_iJsRenderService->removeAllMyTasks(CLIENT_ID);
    
    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iJsRenderService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }

  TEST_F(SchedulerTesting, scheduleTaskPeriodic)
  {
    using namespace rapidjson;
    using namespace chrono;

    Document doc1;
    Pointer("/item").Set(doc1, VAL1);

    //prepare start time
    system_clock::time_point tp = system_clock::now();
    tp += milliseconds(10000); //start in 10 sec
    string tpStr = encodeTimestamp(tp);

    //schedule
    ISchedulerService::TaskHandle th1 = m_iJsRenderService->scheduleTaskPeriodic(CLIENT_ID, doc1, seconds(PERIOD1), tp);
    const rapidjson::Value *task1 = m_iJsRenderService->getMyTask(CLIENT_ID, th1);

    //verify returned task1
    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    //verify returned task1 timeSpec
    const rapidjson::Value *timeSpec = m_iJsRenderService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, "", false, true, PERIOD1 * 1000, tpStr);


    //remove tasks by id, hndl
    m_iJsRenderService->removeTask(CLIENT_ID, th1);

    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iJsRenderService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }

  TEST_F(SchedulerTesting, scheduleTaskHandler)
  {
    using namespace rapidjson;
    Document doc1;
    Pointer("/item").Set(doc1, VAL1);
    
    //schedule
    ISchedulerService::TaskHandle th1 = m_iJsRenderService->scheduleTask(CLIENT_ID, doc1, CRON1);

    { //verify 1st iter
      Document doc = fetchTask(2000);
      //cout << ">>>>>>>>>>>>> " << SchedulerTesting::JsonToStr(&doc) << std::endl;
      ASSERT_FALSE(doc.IsNull());

      //verify task delivered by task handler
      const rapidjson::Value *val1 = Pointer("/item").Get(doc);
      ASSERT_NE(nullptr, val1);
      ASSERT_TRUE(val1->IsInt());
      EXPECT_EQ(VAL1, val1->GetInt());
    }

    { //verify 2nd iter
      Document doc = fetchTask(2000);
      ASSERT_FALSE(doc.IsNull());

      //verify task delivered by task handler
      const rapidjson::Value *val1 = Pointer("/item").Get(doc);
      ASSERT_NE(nullptr, val1);
      ASSERT_TRUE(val1->IsInt());
      EXPECT_EQ(VAL1, val1->GetInt());
    }

    //remove tasks by id, hndl
    m_iJsRenderService->removeTask(CLIENT_ID, th1);

    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iJsRenderService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }

  TEST_F(SchedulerTesting, scheduleTaskAtHandler)
  {
    using namespace rapidjson;
    using namespace chrono;

    Document doc1;
    Pointer("/item").Set(doc1, VAL1);

    system_clock::time_point tp = system_clock::now();
    tp += milliseconds(1000); //expire in 1 sec
    string tpStr = encodeTimestamp(tp);

    //schedule one shot task
    ISchedulerService::TaskHandle th1 = m_iJsRenderService->scheduleTaskAt(CLIENT_ID, doc1, tp);

    { //shouldn't expire yet
      Document doc = fetchTask(200); //200 ms
      ASSERT_TRUE(doc.IsNull());
    }

    { //verify the only expiration
      Document doc = fetchTask(2000);
      ASSERT_FALSE(doc.IsNull());
      const rapidjson::Value *val1 = Pointer("/item").Get(doc);
      ASSERT_NE(nullptr, val1);
      ASSERT_TRUE(val1->IsInt());
      EXPECT_EQ(VAL1, val1->GetInt());
    }

    //shall be empty now
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iJsRenderService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }

  TEST_F(SchedulerTesting, scheduleTaskPeriodicHandler)
  {
    using namespace rapidjson;
    using namespace chrono;

    Document doc1;
    Pointer("/item").Set(doc1, VAL1);

    //prepare start time
    system_clock::time_point tp = system_clock::now();
    tp += milliseconds(1000); //postpone start by 1 sec
    string tpStr = encodeTimestamp(tp);

    //schedule
    ISchedulerService::TaskHandle th1 = m_iJsRenderService->scheduleTaskPeriodic(CLIENT_ID, doc1, seconds(PERIOD1), tp);
    const rapidjson::Value *task1 = m_iJsRenderService->getMyTask(CLIENT_ID, th1);

    { //shouldn't expire yet
      Document doc = fetchTask(200); //200 ms
      ASSERT_TRUE(doc.IsNull());
    }

    { //verify 1st expiration
      Document doc = fetchTask(2000);
      ASSERT_FALSE(doc.IsNull());
      const rapidjson::Value *val1 = Pointer("/item").Get(doc);
      ASSERT_NE(nullptr, val1);
      ASSERT_TRUE(val1->IsInt());
      EXPECT_EQ(VAL1, val1->GetInt());
    }

    { //verify 2nd expiration
      Document doc = fetchTask(2000);
      ASSERT_FALSE(doc.IsNull());
      const rapidjson::Value *val1 = Pointer("/item").Get(doc);
      ASSERT_NE(nullptr, val1);
      ASSERT_TRUE(val1->IsInt());
      EXPECT_EQ(VAL1, val1->GetInt());
    }

    //remove tasks by id, hndl
    m_iJsRenderService->removeTask(CLIENT_ID, th1);

    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iJsRenderService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }
#endif

}
