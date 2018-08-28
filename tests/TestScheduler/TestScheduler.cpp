#include "TestScheduler.h"
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
#include <algorithm>

#include "iqrf__TestScheduler.hxx"

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
    iqrf::ISchedulerService* m_iSchedulerService = nullptr;
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

    void attachInterface(iqrf::ISchedulerService* iface)
    {
      m_iSchedulerService = iface;
    }

    void detachInterface(iqrf::ISchedulerService* iface)
    {
      if (m_iSchedulerService == iface) {
        m_iSchedulerService = nullptr;
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
  TestScheduler::TestScheduler()
  {
  }

  TestScheduler::~TestScheduler()
  {
  }

  void TestScheduler::activate(const shape::Properties *props)
  {
    Imp::get().activate(props);
  }

  void TestScheduler::deactivate()
  {
    Imp::get().deactivate();
  }

  void TestScheduler::modify(const shape::Properties *props)
  {
  }

  void TestScheduler::attachInterface(iqrf::ISchedulerService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestScheduler::detachInterface(iqrf::ISchedulerService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestScheduler::attachInterface(shape::ILaunchService* iface)
  {
    Imp::get().attachInterface(iface);
  }

  void TestScheduler::detachInterface(shape::ILaunchService* iface)
  {
    Imp::get().detachInterface(iface);
  }

  void TestScheduler::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void TestScheduler::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  ////////////////////////////////////////////////////////
  class SchedulerTesting : public ::testing::Test
  {
  protected:
    const std::string CLIENT_ID = "TestScheduler";
    const int VAL1 = 1;
    const int VAL2 = 2;
    const std::string CRON1 = "*/1 * * * * * *"; //every 1sec
    const std::string CRON2 = "20 * * * * * *";
    const int PERIOD1 = 1; //every 1sec

    iqrf::ISchedulerService* m_iSchedulerService = nullptr;
    std::condition_variable m_msgCon;
    std::mutex m_mux;
    rapidjson::Document m_expectedTask;

    void SetUp(void) override
    {
      ASSERT_NE(nullptr, &Imp::get());
      ASSERT_NE(nullptr, Imp::get().m_iSchedulerService);
      m_iSchedulerService = Imp::get().m_iSchedulerService;
      m_iSchedulerService->registerTaskHandler(CLIENT_ID, [&](const rapidjson::Value& task) { taskHandler(task); });
      ASSERT_NE(nullptr, &Imp::get().m_iLaunchService);
    };

    void TearDown(void) override
    {
      m_iSchedulerService->unregisterTaskHandler(CLIENT_ID);
    };

    void taskHandler(const rapidjson::Value& task)
    {
      TRC_FUNCTION_ENTER("");

      std::unique_lock<std::mutex> lck(m_mux);
      m_expectedTask.CopyFrom(task, m_expectedTask.GetAllocator());
      m_msgCon.notify_all();

      TRC_FUNCTION_LEAVE("");
    }

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

    void checkTimeSpec(
      const rapidjson::Value *timeSpec,
      const std::string& cronTimeStr,
      bool exactTimeB,
      bool periodicB,
      int periodI,
      const std::string& startTimeStr
      )
    {
      TRC_FUNCTION_ENTER(PAR(cronTimeStr) << PAR(exactTimeB) << PAR(periodicB) << PAR(periodI) << PAR(startTimeStr));

      using namespace rapidjson;
      using namespace chrono;

      const rapidjson::Value *cronTime = Pointer("/cronTime").Get(*timeSpec);
      const rapidjson::Value *exactTime = Pointer("/exactTime").Get(*timeSpec);
      const rapidjson::Value *periodic = Pointer("/periodic").Get(*timeSpec);
      const rapidjson::Value *period = Pointer("/period").Get(*timeSpec);
      const rapidjson::Value *startTime = Pointer("/startTime").Get(*timeSpec);

      ASSERT_NE(nullptr, cronTime);
      ASSERT_NE(nullptr, exactTime);
      ASSERT_NE(nullptr, periodic);
      ASSERT_NE(nullptr, period);
      ASSERT_NE(nullptr, startTime);

      ASSERT_TRUE(cronTime->IsString());
      ASSERT_TRUE(exactTime->IsBool());
      ASSERT_TRUE(periodic->IsBool());
      ASSERT_TRUE(period->IsInt());
      ASSERT_TRUE(startTime->IsString());

      EXPECT_EQ(cronTimeStr, cronTime->GetString());
      EXPECT_EQ(exactTimeB, exactTime->GetBool());
      EXPECT_EQ(periodicB, periodic->GetBool());
      EXPECT_EQ(periodI, period->GetInt());

      //std::cout << ">>>>>>>>> " << SchedulerTesting::JsonToStr(startTime) << std::endl;
      if (!startTimeStr.empty()) {
        EXPECT_EQ(startTimeStr, startTime->GetString());
      }

      TRC_FUNCTION_LEAVE("");
    }

    rapidjson::Document fetchTask(unsigned millisToWait)
    {
      TRC_FUNCTION_ENTER(PAR(millisToWait));

      std::unique_lock<std::mutex> lck(m_mux);
      if (m_expectedTask.IsNull()) {
        while (m_msgCon.wait_for(lck, std::chrono::milliseconds(millisToWait)) != std::cv_status::timeout) {
          if (!m_expectedTask.IsNull()) break;
        }
      }
      
      rapidjson::Document expectedTask;
      expectedTask.Swap(m_expectedTask);
      TRC_FUNCTION_LEAVE("");
      return expectedTask;
    }

  };

  /////////// Tests
  //TODO missing:
  // exact timing of tasks with long time => Scheduler wouldn't call now() directly. It shall be done by time provider interface mocked in tests
  // cron NICKNAMES: @yearly, ...
  // predefined scheduler/tasks.json => shall be done after implementation of persistency

  TEST_F(SchedulerTesting, empty)
  {
    //verify empty result as we haven't any tasks yet
    std::vector<ISchedulerService::TaskHandle> taskHandleVect =  m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());

    //verify empty result as we pass wrong handler
    const rapidjson::Value *val = m_iSchedulerService->getMyTask(CLIENT_ID, 0);
    EXPECT_EQ(nullptr, val);

    //verify empty result as we pass wrong handler
    val = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, 0);
    EXPECT_EQ(nullptr, val);
  }

  TEST_F(SchedulerTesting, scheduleTask)
  {
    using namespace rapidjson;
    Document doc1;
    Pointer("/item").Set(doc1, VAL1);
    Document doc2;
    Pointer("/item").Set(doc2, VAL2);

    //schedule two tasks by cron time
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTask(CLIENT_ID, doc1, CRON1);
    ISchedulerService::TaskHandle th2 = m_iSchedulerService->scheduleTask(CLIENT_ID, doc2, CRON2);

    //expected two handlers
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    ASSERT_EQ(2, taskHandleVect.size());
    //tasks ordered according scheduled time so no exact ordering expected
    EXPECT_TRUE(th1 == taskHandleVect[0] || th1 == taskHandleVect[1]);
    EXPECT_TRUE(th2 == taskHandleVect[0] || th2 == taskHandleVect[1]);

    //verify returned task1
    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);
    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    //verify returned task2
    const rapidjson::Value *task2 = m_iSchedulerService->getMyTask(CLIENT_ID, th2);
    const rapidjson::Value *val2 = Pointer("/item").Get(*task2);
    ASSERT_NE(nullptr, val2);
    ASSERT_TRUE(val2->IsInt());
    EXPECT_EQ(VAL2, val2->GetInt());

    //verify returned task1 timeSpec
    const rapidjson::Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, CRON1, false, false, 0, "");

    //remove tasks by vector
    std::vector<ISchedulerService::TaskHandle> taskHandleVectRem = { th1, th2 };
    m_iSchedulerService->removeTasks(CLIENT_ID, taskHandleVectRem);
    
    //verify removal
    taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
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
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTaskAt(CLIENT_ID, doc1, tp);
    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);

    //verify returned task1
    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    //verify returned task1 timeSpec
    const rapidjson::Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, "", true, false, 0, tpStr); //expiration time as str in tpStr

    //remove tasks by id
    m_iSchedulerService->removeAllMyTasks(CLIENT_ID);
    
    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
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
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTaskPeriodic(CLIENT_ID, doc1, seconds(PERIOD1), tp);
    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);

    //verify returned task1
    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    //verify returned task1 timeSpec
    const rapidjson::Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, "", false, true, PERIOD1 * 1000, tpStr);


    //remove tasks by id, hndl
    m_iSchedulerService->removeTask(CLIENT_ID, th1);

    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }

  TEST_F(SchedulerTesting, scheduleTaskHandler)
  {
    using namespace rapidjson;
    Document doc1;
    Pointer("/item").Set(doc1, VAL1);
    
    //schedule
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTask(CLIENT_ID, doc1, CRON1);

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
    m_iSchedulerService->removeTask(CLIENT_ID, th1);

    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
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
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTaskAt(CLIENT_ID, doc1, tp);

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
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
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
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTaskPeriodic(CLIENT_ID, doc1, seconds(PERIOD1), tp);
    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);

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
    m_iSchedulerService->removeTask(CLIENT_ID, th1);

    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }


}
