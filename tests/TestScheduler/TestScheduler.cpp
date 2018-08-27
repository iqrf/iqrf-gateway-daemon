#include "TestScheduler.h"
#include "Trace.h"
#include "GTestStaticRunner.h"
#include "gtest/gtest.h"
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

    void taskHandler(const rapidjson::Value& task)
    {
      TRC_FUNCTION_ENTER("");
      TRC_FUNCTION_LEAVE("")
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

  };

  TEST_F(SchedulerTesting, TestEmpty)
  {
    std::vector<ISchedulerService::TaskHandle> taskHandleVect =  m_iSchedulerService->getMyTasks(CLIENT_ID);
    EXPECT_EQ(0, taskHandleVect.size());
    const rapidjson::Value *val = m_iSchedulerService->getMyTask(CLIENT_ID, 0);
    EXPECT_EQ(nullptr, val);
  }

}
