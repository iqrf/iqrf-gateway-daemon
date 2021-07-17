/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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
#include <condition_variable>
#include <algorithm>

#include "iqrf__TestScheduler.hxx"

TRC_INIT_MNAME(iqrf::TestScheduler)

using namespace std;

namespace iqrf {

  class Imp {
  private:
    Imp()
    {
    }

  public:
    shape::ILaunchService* m_iLaunchService = nullptr;
    iqrf::ISchedulerService* m_iJsRenderService = nullptr;
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
      (void)props; //silence -Wunused-parameter
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
      m_iJsRenderService = iface;
    }

    void detachInterface(iqrf::ISchedulerService* iface)
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
    (void)props; //silence -Wunused-parameter
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
    const std::string CLIENT_ID_PERSIST = "TestSchedulerPersist";
    const std::string CLIENT_ID_PERSIST_PERIODIC = "TestSchedulerPersistPeriodic";
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
      ASSERT_NE(nullptr, Imp::get().m_iJsRenderService);
      m_iSchedulerService = Imp::get().m_iJsRenderService;
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

      ASSERT_TRUE(cronTime->IsArray());
      ASSERT_TRUE(exactTime->IsBool());
      ASSERT_TRUE(periodic->IsBool());
      ASSERT_TRUE(period->IsInt());
      ASSERT_TRUE(startTime->IsString());

      std::ostringstream os;
      os <<
        Pointer("/cronTime/0").Get(*timeSpec)->GetString() << ' ' <<
        Pointer("/cronTime/1").Get(*timeSpec)->GetString() << ' ' <<
        Pointer("/cronTime/2").Get(*timeSpec)->GetString() << ' ' <<
        Pointer("/cronTime/3").Get(*timeSpec)->GetString() << ' ' <<
        Pointer("/cronTime/4").Get(*timeSpec)->GetString() << ' ' <<
        Pointer("/cronTime/5").Get(*timeSpec)->GetString() << ' ' <<
        Pointer("/cronTime/6").Get(*timeSpec)->GetString();

      //EXPECT_EQ(cronTimeStr, cronTime->GetString());
      EXPECT_EQ(cronTimeStr, os.str());
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

  TEST_F(SchedulerTesting, empty)
  {
    //verify empty result as we haven't any tasks yet
    std::vector<ISchedulerService::TaskHandle> taskHandleVect =  m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ((size_t)0, taskHandleVect.size());

    //verify empty result as we pass wrong handler
    const rapidjson::Value *val = m_iSchedulerService->getMyTask(CLIENT_ID, 0);
    EXPECT_EQ(nullptr, val);

    //verify empty result as we pass wrong handler
    val = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, 0);
    EXPECT_EQ(nullptr, val);
  }

  TEST_F(SchedulerTesting, persist)
  {
    using namespace rapidjson;

    const int TID1 = 1736;
    const int TID2 = 25828;
    //const int TID3 = 78963;
    const std::string MSG_ID1 = "b726ecb9-ee7c-433a-9aa4-3fb21cae2d4d";
    const std::string MSG_ID2 = "a726ecb9-ee7c-433a-9aa4-3fb21cae2d4d";
    //const std::string MSG_ID3 = "f726ecb9-ee7c-433a-9aa4-3fb21cae2d4d";

    //verify result
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID_PERSIST);
    ASSERT_EQ((size_t)2, taskHandleVect.size());

    ISchedulerService::TaskHandle th1 = taskHandleVect[0];
    ISchedulerService::TaskHandle th2 = taskHandleVect[1];

    //tasks ordered according scheduled time so keep order
    if (th1 != TID1) {
      //swap
      ISchedulerService::TaskHandle tmp = th1;
      th1 = th2;
      th2 = tmp;
    }

    //tasks ordered according scheduled time so no exact ordering expected
    EXPECT_TRUE(TID1 == th1);
    EXPECT_TRUE(TID2 == th2);

    //verify returned task1
    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID_PERSIST, th1);
    const rapidjson::Value *val1 = Pointer("/message/data/msgId").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsString());
    EXPECT_EQ(MSG_ID1, val1->GetString());

    //verify returned task2
    const rapidjson::Value *task2 = m_iSchedulerService->getMyTask(CLIENT_ID_PERSIST, th2);
    const rapidjson::Value *val2 = Pointer("/message/data/msgId").Get(*task2);
    ASSERT_NE(nullptr, val2);
    ASSERT_TRUE(val2->IsString());
    EXPECT_EQ(MSG_ID2, val2->GetString());
  }

  TEST_F(SchedulerTesting, persistPeriodic)
  {
    using namespace rapidjson;

    const int TID3 = 78963;
    const std::string MSG_ID3 = "f726ecb9-ee7c-433a-9aa4-3fb21cae2d4d";

    //verify result
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID_PERSIST_PERIODIC);
    ASSERT_EQ((size_t)1, taskHandleVect.size());

    ISchedulerService::TaskHandle th3 = taskHandleVect[0];

    EXPECT_TRUE(TID3 == th3);

    //verify returned task3
    const rapidjson::Value *task3 = m_iSchedulerService->getMyTask(CLIENT_ID_PERSIST_PERIODIC, th3);
    const rapidjson::Value *val3 = Pointer("/message/data/msgId").Get(*task3);
    ASSERT_NE(nullptr, val3);
    ASSERT_TRUE(val3->IsString());
    EXPECT_EQ(MSG_ID3, val3->GetString());
  }

  TEST_F(SchedulerTesting, addTaskPersist)
  {
    using namespace rapidjson;
    Document doc1;
    Pointer("/item").Set(doc1, VAL1);

    //schedule task by cron time
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTask(CLIENT_ID, doc1, CRON1, true);

    //expected one handler
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    ASSERT_EQ((size_t)1, taskHandleVect.size());
    //tasks ordered according scheduled time so no exact ordering expected
    EXPECT_TRUE(th1 == taskHandleVect[0]);

    //verify returned task1
    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);
    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    //verify returned task1 timeSpec
    const rapidjson::Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, CRON1, false, false, 0, "");

    //verify created persistent file
    std::ostringstream os;
    os << "scheduler/" << th1 << ".json";
    std::string fname = os.str();

    std::ifstream ifs(fname), ifs1;
    ASSERT_TRUE(ifs.is_open());

    //verify file written correctly
    Document d;
    IStreamWrapper isw(ifs);
    d.ParseStream(isw);
    const rapidjson::Value *val2 = Pointer("/task/item").Get(d);
    ASSERT_NE(nullptr, val2);
    ASSERT_TRUE(val2->IsInt());
    EXPECT_EQ(VAL1, val2->GetInt());
    
    ifs.close();

    //remove tasks
    m_iSchedulerService->removeTask(CLIENT_ID, th1);

    //verify remove file
    ifs1.open(fname);
    ASSERT_FALSE(ifs1.good());
    ifs1.close();
  }

  TEST_F(SchedulerTesting, addTaskAtPersist)
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
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTaskAt(CLIENT_ID, doc1, tp, true);
    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);

    //verify returned task1
    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    //verify returned task1 timeSpec
    const rapidjson::Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, "      ", true, false, 0, tpStr); //expiration time as str in tpStr

    //verify created persistent file
    std::ostringstream os;
    os << "scheduler/" << th1 << ".json";
    std::string fname = os.str();

    std::ifstream ifs(fname), ifs1;
    ASSERT_TRUE(ifs.is_open());

    //verify file written correctly
    Document d;
    IStreamWrapper isw(ifs);
    d.ParseStream(isw);
    const rapidjson::Value *val2 = Pointer("/task/item").Get(d);
    ASSERT_NE(nullptr, val2);
    ASSERT_TRUE(val2->IsInt());
    EXPECT_EQ(VAL1, val2->GetInt());

    ifs.close();

    //remove tasks
    m_iSchedulerService->removeTask(CLIENT_ID, th1);

    //verify remove file
    ifs1.open(fname);
    ASSERT_FALSE(ifs1.good());
    ifs1.close();
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
    ASSERT_EQ((size_t)2, taskHandleVect.size());
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
    EXPECT_EQ((size_t)0, taskHandleVect.size());
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
    SchedulerTesting::checkTimeSpec(timeSpec, "      ", true, false, 0, tpStr); //expiration time as str in tpStr

    //remove tasks by id
    m_iSchedulerService->removeAllMyTasks(CLIENT_ID);
    
    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ((size_t)0, taskHandleVect.size());
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
    SchedulerTesting::checkTimeSpec(timeSpec, "      ", false, true, PERIOD1, tpStr);

    //remove tasks by id, hndl
    m_iSchedulerService->removeTask(CLIENT_ID, th1);

    //verify removal
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ((size_t)0, taskHandleVect.size());
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
    EXPECT_EQ((size_t)0, taskHandleVect.size());
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
    m_iSchedulerService->scheduleTaskAt(CLIENT_ID, doc1, tp);

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
    EXPECT_EQ((size_t)0, taskHandleVect.size());
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
    //const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);

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
    EXPECT_EQ((size_t)0, taskHandleVect.size());
  }


}
