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
#include <Windows.h>

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
    const std::string CRON1 = "2 * * * * * *";
    const std::string CRON2 = "20 * * * * * *";
    const int PERIOD1 = 10;

    iqrf::ISchedulerService* m_iSchedulerService = nullptr;

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
      TRC_FUNCTION_LEAVE("")
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
    }

  };

  TEST_F(SchedulerTesting, empty)
  {
    std::vector<ISchedulerService::TaskHandle> taskHandleVect =  m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());

    const rapidjson::Value *val = m_iSchedulerService->getMyTask(CLIENT_ID, 0);
    EXPECT_EQ(nullptr, val);

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
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTask(CLIENT_ID, doc1, CRON1);
    ISchedulerService::TaskHandle th2 = m_iSchedulerService->scheduleTask(CLIENT_ID, doc2, CRON2);

    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    ASSERT_EQ(2, taskHandleVect.size());
    //tasks ordered according scheduled time
    EXPECT_TRUE(th1 == taskHandleVect[0] || th1 == taskHandleVect[1]);
    EXPECT_TRUE(th2 == taskHandleVect[0] || th2 == taskHandleVect[1]);

    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);
    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    const rapidjson::Value *task2 = m_iSchedulerService->getMyTask(CLIENT_ID, th2);
    const rapidjson::Value *val2 = Pointer("/item").Get(*task2);
    ASSERT_NE(nullptr, val2);
    ASSERT_TRUE(val2->IsInt());
    EXPECT_EQ(VAL2, val2->GetInt());

    const rapidjson::Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, CRON1, false, false, 0, "");

    std::vector<ISchedulerService::TaskHandle> taskHandleVectRem = { th1, th2 };
    m_iSchedulerService->removeTasks(CLIENT_ID, taskHandleVectRem);
    taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }

  TEST_F(SchedulerTesting, scheduleTaskAt)
  {
    using namespace rapidjson;
    using namespace chrono;

    Document doc1;
    Pointer("/item").Set(doc1, VAL1);

    system_clock::time_point tp = system_clock::now();
    tp += milliseconds(10000); //expire in 10 sec
    string tpStr = encodeTimestamp(tp);

    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTaskAt(CLIENT_ID, doc1, tp);
    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);

    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    const rapidjson::Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, "", true, false, 0, tpStr);

    m_iSchedulerService->removeAllMyTasks(CLIENT_ID);
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }

  TEST_F(SchedulerTesting, scheduleTaskPeriodic)
  {
    using namespace rapidjson;
    using namespace chrono;

    Document doc1;
    Pointer("/item").Set(doc1, VAL1);

    system_clock::time_point tp = system_clock::now();
    tp += milliseconds(10000); //start in 10 sec
    string tpStr = encodeTimestamp(tp);

    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTaskPeriodic(CLIENT_ID, doc1, seconds(PERIOD1), tp);
    const rapidjson::Value *task1 = m_iSchedulerService->getMyTask(CLIENT_ID, th1);

    const rapidjson::Value *val1 = Pointer("/item").Get(*task1);
    ASSERT_NE(nullptr, val1);
    ASSERT_TRUE(val1->IsInt());
    EXPECT_EQ(VAL1, val1->GetInt());

    const rapidjson::Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(CLIENT_ID, th1);
    SchedulerTesting::checkTimeSpec(timeSpec, "", false, true, PERIOD1 * 1000, tpStr);

    m_iSchedulerService->removeTask(CLIENT_ID, th1);
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }

  TEST_F(SchedulerTesting, scheduleTaskHandler)
  {
    using namespace rapidjson;
    Document doc1;
    Pointer("/item").Set(doc1, VAL1);
    ISchedulerService::TaskHandle th1 = m_iSchedulerService->scheduleTask(CLIENT_ID, doc1, CRON1);

    m_iSchedulerService->removeTask(CLIENT_ID, th1);
    std::vector<ISchedulerService::TaskHandle> taskHandleVect = m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
  }


}
