/**
 * Copyright 2016-2017 MICRORISC s.r.o.
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
#define ISchedulerService_EXPORTS

#include "Scheduler.h"
#include "Trace.h"
#include "ShapeDefines.h"
#include <algorithm>

#include "iqrf__Scheduler.hxx"

TRC_INIT_MODULE(iqrf::Scheduler);

using namespace std::chrono;

namespace iqrf {

  Scheduler::Scheduler()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  Scheduler::~Scheduler()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  void Scheduler::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "Scheduler instance activate" << std::endl <<
      "******************************"
    );

    std::string fname;
    if (shape::Properties::Result::ok == props->getMemberAsString("TasksFile", fname)) {
      updateConfiguration(fname);
    }

    m_dpaTaskQueue = shape_new TaskQueue<ScheduleRecord>([&](const ScheduleRecord& record) {
      handleScheduledRecord(record);
    });

    m_scheduledTaskPushed = false;
    m_runTimerThread = true;
    m_timerThread = std::thread(&Scheduler::timer, this);

    TRC_INFORMATION("Scheduler started");

    TRC_FUNCTION_LEAVE("")
  }

  void Scheduler::deactivate()
  {
    TRC_FUNCTION_ENTER("");

    {
      m_runTimerThread = false;
      std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
      m_scheduledTaskPushed = true;
      m_conditionVariable.notify_one();
    }

    m_dpaTaskQueue->stopQueue();

    if (m_timerThread.joinable()) {
      TRC_DEBUG("Joining scheduler thread");
      m_timerThread.join();
      TRC_DEBUG("scheduler thread joined");
    }

    TRC_DEBUG("Try to destroy: " << PAR(m_dpaTaskQueue->size()));
    delete m_dpaTaskQueue;
    m_dpaTaskQueue = nullptr;

    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "Scheduler instance deactivate" << std::endl <<
      "******************************"
    );

    TRC_FUNCTION_LEAVE("")
  }

  void Scheduler::modify(const shape::Properties *props)
  {
  }

  void Scheduler::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void Scheduler::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void Scheduler::updateConfiguration(const std::string& fname)
  {
    TRC_FUNCTION_ENTER("");

    rapidjson::Document cfg;
    jutils::parseJsonFile(fname, cfg);
    jutils::assertIsObject("", cfg);

    std::vector<std::shared_ptr<ScheduleRecord>> tempRecords;

    const auto m = cfg.FindMember("TasksJson");

    if (m == cfg.MemberEnd()) { //old textual form
      std::vector<std::string> records = jutils::getMemberAsVector<std::string>("Tasks", cfg);
      for (auto & it : records) {
        try {
          tempRecords.push_back(std::shared_ptr<ScheduleRecord>(shape_new ScheduleRecord(it)));
        }
        catch (std::exception &e) {
          CATCH_EXC_TRC_WAR(std::exception, e, "Cought when parsing scheduler table");
        }
      }
    }
    else { //enhanced Json form

      try {
        const auto v = jutils::getMember("TasksJson", cfg);
        if (!v->value.IsArray())
          THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array, detected: " << NAME_PAR(name, v->value.GetString()) << NAME_PAR(type, v->value.GetType()));

        for (auto it = v->value.Begin(); it != v->value.End(); ++it) {
          tempRecords.push_back(std::shared_ptr<ScheduleRecord>(shape_new ScheduleRecord(*it)));
        }
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Cought when parsing scheduler table");
      }
    }

    addScheduleRecords(tempRecords);

    TRC_FUNCTION_LEAVE("");
  }

  //void Scheduler::start()
  //{
  //  TRC_FUNCTION_ENTER("");

  //  //m_dpaTaskQueue = shape_new TaskQueue<ScheduleRecord>([&](const ScheduleRecord& record) {
  //  //  handleScheduledRecord(record);
  //  //});

  //  //m_scheduledTaskPushed = false;
  //  //m_runTimerThread = true;
  //  //m_timerThread = std::thread(&Scheduler::timer, this);

  //  //TRC_INFORMATION("Scheduler started");

  //  TRC_FUNCTION_LEAVE("");
  //}

  //void Scheduler::stop()
  //{
  //  TRC_FUNCTION_ENTER("");
  //  //{
  //  //  m_runTimerThread = false;
  //  //  std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
  //  //  m_scheduledTaskPushed = true;
  //  //  m_conditionVariable.notify_one();
  //  //}

  //  //m_dpaTaskQueue->stopQueue();

  //  TRC_INFORMATION("Scheduler stopped");
  //  TRC_FUNCTION_LEAVE("");
  //}

  Scheduler::TaskHandle Scheduler::scheduleTaskAt(const std::string& clientId, const std::string& task, const std::chrono::system_clock::time_point& tp)
  {
    std::shared_ptr<ScheduleRecord> s = std::shared_ptr<ScheduleRecord>(shape_new ScheduleRecord(clientId, task, tp));
    return addScheduleRecord(s);
  }

  Scheduler::TaskHandle Scheduler::scheduleTaskPeriodic(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
    const std::chrono::system_clock::time_point& tp)
  {
    std::shared_ptr<ScheduleRecord> s = std::shared_ptr<ScheduleRecord>(shape_new ScheduleRecord(clientId, task, sec, tp));
    return addScheduleRecord(s);
  }

  int Scheduler::handleScheduledRecord(const ScheduleRecord& record)
  {
    //TRC_DEBUG("==================================" << std::endl <<
    //  "Scheduled msg: " << std::endl << FORM_HEX(record.getTask().data(), record.getTask().size()));

    {
      std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
      auto found = m_messageHandlers.find(record.getClientId());
      if (found != m_messageHandlers.end()) {
        //TRC_DEBUG(NAME_PAR(Task, record.getTask()) << " has been passed to: " << NAME_PAR(ClinetId, record.getClientId()));
        found->second(record.getTask());
      }
      else {
        TRC_DEBUG("Unregistered client: " << PAR(record.getClientId()));
      }
    }

    return 0;
  }

  Scheduler::TaskHandle Scheduler::addScheduleRecordUnlocked(std::shared_ptr<ScheduleRecord>& record)
  {
    system_clock::time_point timePoint;
    std::tm timeStr;
    ScheduleRecord::getTime(timePoint, timeStr);
    TRC_DEBUG(ScheduleRecord::asString(timePoint));

    //add according time
    system_clock::time_point tp = record->getNext(timePoint, timeStr);
    m_scheduledTasksByTime.insert(std::make_pair(tp, record));

    //add according handle
    while (true) {//get unique handle
      auto result = m_scheduledTasksByHandle.insert(std::make_pair(record->getTaskHandle(), record));
      if (result.second)
        break;
      else
        shuffleDuplicitHandle(*record);
    }

    return record->getTaskHandle();
  }

  Scheduler::TaskHandle Scheduler::addScheduleRecord(std::shared_ptr<ScheduleRecord>& record)
  {
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);

    addScheduleRecordUnlocked(record);

    // notify timer thread
    std::unique_lock<std::mutex> lckn(m_conditionVariableMutex);
    m_scheduledTaskPushed = true;
    m_conditionVariable.notify_one();

    return record->getTaskHandle();
  }

  void Scheduler::addScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records)
  {
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);

    for (auto & record : records) {
      addScheduleRecordUnlocked(record);
    }

    // notify timer thread
    std::unique_lock<std::mutex> lckn(m_conditionVariableMutex);
    m_scheduledTaskPushed = true;
    m_conditionVariable.notify_one();
  }

  void Scheduler::removeScheduleRecordUnlocked(std::shared_ptr<ScheduleRecord>& record)
  {
    Scheduler::TaskHandle handle = record->getTaskHandle();
    for (auto it = m_scheduledTasksByTime.begin(); it != m_scheduledTasksByTime.end(); ) {
      if (it->second->getTaskHandle() == handle)
        it = m_scheduledTasksByTime.erase(it);
      else
        it++;
    }
    m_scheduledTasksByHandle.erase(handle);
  }

  void Scheduler::removeScheduleRecord(std::shared_ptr<ScheduleRecord>& record)
  {
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    removeScheduleRecordUnlocked(record);
  }

  void Scheduler::removeScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records)
  {
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    for (auto & record : records) {
      removeScheduleRecordUnlocked(record);
    }
  }

  //thread function
  void Scheduler::timer()
  {
    system_clock::time_point timePoint;
    std::tm timeStr;
    ScheduleRecord::getTime(timePoint, timeStr);
    TRC_DEBUG(ScheduleRecord::asString(timePoint));

    while (m_runTimerThread) {

      { // wait for something in the m_scheduledTasks;
        std::unique_lock<std::mutex> lck(m_conditionVariableMutex);
        m_conditionVariable.wait_until(lck, timePoint, [&] { return m_scheduledTaskPushed; });
        m_scheduledTaskPushed = false;
      }

      // get actual time
      ScheduleRecord::getTime(timePoint, timeStr);

      // fire all expired tasks
      while (m_runTimerThread) {

        m_scheduledTasksMutex.lock();

        if (m_scheduledTasksByTime.empty()) {
          nextWakeupAndUnlock(timePoint);
          break;
        }

        auto begin = m_scheduledTasksByTime.begin();
        std::shared_ptr<ScheduleRecord> record = begin->second;
        auto diff = begin->first.time_since_epoch().count() - timePoint.time_since_epoch().count();

        if (begin->first < timePoint) {

          // erase fired
          m_scheduledTasksByTime.erase(begin);

          // get and schedule next
          system_clock::time_point nextTimePoint = record->getNext(timePoint, timeStr);
          if (nextTimePoint >= timePoint) {
            m_scheduledTasksByTime.insert(std::make_pair(nextTimePoint, record));
          }
          else {
            //TODO remove from m_scheduledTasksByHandle
          }

          nextWakeupAndUnlock(timePoint);

          if (record->verifyTimePattern(timeStr)) {
            // fire
            //TRC_INF("Task fired at: " << ScheduleRecord::asString(timePoint) << PAR(record->getTask()));
            m_dpaTaskQueue->pushToQueue(*record); //copy record
          }

        }
        else {
          nextWakeupAndUnlock(timePoint);
          break;
        }
      }
    }
  }

  void Scheduler::nextWakeupAndUnlock(system_clock::time_point& timePoint)
  {
    // get next wakeup time
    if (!m_scheduledTasksByTime.empty()) {
      timePoint = m_scheduledTasksByTime.begin()->first;
    }
    else {
      timePoint += seconds(10);
    }
    //TRC_DEBUG("UNLOCKING MUTEX");
    m_scheduledTasksMutex.unlock();
  }

  void Scheduler::registerMessageHandler(const std::string& clientId, TaskHandlerFunc fun)
  {
    std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
    //TODO check success
    m_messageHandlers.insert(make_pair(clientId, fun));
  }

  void Scheduler::unregisterMessageHandler(const std::string& clientId)
  {
    std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
    m_messageHandlers.erase(clientId);
  }

  std::vector<std::string> Scheduler::getMyTasks(const std::string& clientId) const
  {
    std::vector<std::string> retval;
    // lock and copy
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    for (auto & task : m_scheduledTasksByTime) {
      if (task.second->getClientId() == clientId) {
        retval.push_back(task.second->getTask());
      }
    }
    return retval;
  }

  std::string Scheduler::getMyTask(const std::string& clientId, const TaskHandle& hndl) const
  {
    std::string retval;
    // lock and copy
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    auto found = m_scheduledTasksByHandle.find(hndl);
    if (found != m_scheduledTasksByHandle.end() && clientId == found->second->getClientId())
      retval = found->second->getTask();
    return retval;
  }

  void Scheduler::removeAllMyTasks(const std::string& clientId)
  {
    // lock and remove
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    for (auto it = m_scheduledTasksByTime.begin(); it != m_scheduledTasksByTime.end(); ) {
      if (it->second->getClientId() == clientId) {
        m_scheduledTasksByHandle.erase(it->second->getTaskHandle());
        it = m_scheduledTasksByTime.erase(it);
      }
      else
        it++;
    }
  }

  void Scheduler::removeTask(const std::string& clientId, TaskHandle hndl)
  {
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    auto found = m_scheduledTasksByHandle.find(hndl);
    if (found != m_scheduledTasksByHandle.end() && clientId == found->second->getClientId())
      removeScheduleRecordUnlocked(found->second);
  }

  void Scheduler::removeTasks(const std::string& clientId, std::vector<TaskHandle> hndls)
  {
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    for (auto& it : hndls) {
      auto found = m_scheduledTasksByHandle.find(it);
      if (found != m_scheduledTasksByHandle.end() && clientId == found->second->getClientId())
        removeScheduleRecordUnlocked(found->second);
    }
  }

}