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

#pragma once

#include "JsonUtils.h"
#include "ScheduleRecord.h"
#include "TaskQueue.h"
#include "ISchedulerService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"

#include <string>
#include <chrono>
#include <map>
#include <memory>

/// \class Scheduler
/// \brief Tasks scheduler
/// \details
/// Scheduled tasks are set according a configuration file or during runtime via IScheduler interface.
/// The tasks are scheduled periodically or according a time pattern as defined by Cron syntax.
/// The tasks are stored as std::string and delivered to clients via registered callbacks.
/// It is up to the client to parse content of delivered std::string and handle it approproiatelly.
/// Handling must not block as it is done in the scheduler tasks queue and would block handling of other fired tasks.
/// The client shall create its own handling thread in case of blocking processing.
///
/// Runtime scheduling is possible via methods scheduleTaskAt() for task firing at exact time point in future or scheduleTaskPeriodic()
/// for periodic firing with period in soconds.
///
/// Scheduling via configuration file allows scheduling according time pattern similar to unix Cron.
/// See for details e.g: https://en.wikipedia.org/wiki/Cron link.
/// Time pattern consist of seven tokens. Unlike Cron it has the token for seconds: 
/// "sec min hour day mon year wday".
/// The tokens are accepted in these Cron forms, e.g:
///
/// Token | meaning
/// ----- | -------
/// * | every
/// 3 | 3rd 
/// 1,2,3 | 1st, 2nd, 3rd
/// 5/* | every divisible by 5
///
/// It is possible to use Cron nicknames for time pattern.
/// - "@reboot": Run once after reboot.
/// - "@yearly": Run once a year, ie.  "0 0 0 0 1 1 *".
/// - "@annually": Run once a year, ie.  "0 0 0 0 1 1 *".
/// - "@monthly": Run once a month, ie. "0 0 0 0 1 * *".
/// - "@weekly": Run once a week, ie.  "0 0 0 * * * 0".
/// - "@daily": Run once a day, ie.   "0 0 0 * * * *".
/// - "@hourly": Run once an hour, ie. "0 0 * * * * *".
/// - "@minutely": Run once a minute, ie. "0 * * * * * *".
///
/// Configuration file is accepted in this JSON format:
/// ```json
/// {
///  "TasksJson" : [                        #tasks array
///   {
///     "time": "*/5 6 * * * * *",          #time pattern
///     "service" : "BaseServiceForMQTT1",  #id of callback registrator
///     "message" : {                       #task (passed as std::string)
///       "ctype": "dpa",
///       "type" : "raw",
///       "msgid" : "1",
///       "timeout" : 1000,
///       "request" : "00.00.06.03.ff.ff",
///       "request_ts" : "",
///       "confirmation" : ".",
///       "confirmation_ts" : "",
///       "response" : ".",
///       "response_ts" : ""
///   }
///  ]
/// }
/// ```

namespace iqrfgw {
  class Scheduler : public ISchedulerService
  {
  public:
    Scheduler();
    virtual ~Scheduler();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

    //void start() override;
    //void stop() override;

    void registerMessageHandler(const std::string& clientId, TaskHandlerFunc fun) override;
    void unregisterMessageHandler(const std::string& clientId) override;

    std::vector<std::string> getMyTasks(const std::string& clientId) const override;
    std::string getMyTask(const std::string& clientId, const TaskHandle& hndl) const override;

    TaskHandle scheduleTaskAt(const std::string& clientId, const std::string& task, const std::chrono::system_clock::time_point& tp) override;
    TaskHandle scheduleTaskPeriodic(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
      const std::chrono::system_clock::time_point& tp) override;

    void removeAllMyTasks(const std::string& clientId) override;
    void removeTask(const std::string& clientId, TaskHandle hndl) override;
    void removeTasks(const std::string& clientId, std::vector<TaskHandle> hndls) override;

    void updateConfiguration(const std::string& fname);

  private:
    int handleScheduledRecord(const ScheduleRecord& record);

    TaskHandle addScheduleRecordUnlocked(std::shared_ptr<ScheduleRecord>& record);
    TaskHandle addScheduleRecord(std::shared_ptr<ScheduleRecord>& record);
    void addScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records);

    void removeScheduleRecordUnlocked(std::shared_ptr<ScheduleRecord>& record);
    void removeScheduleRecord(std::shared_ptr<ScheduleRecord>& record);
    void removeScheduleRecords(std::vector<std::shared_ptr<ScheduleRecord>>& records);

    ////////////////////////////////
    TaskQueue<ScheduleRecord>* m_dpaTaskQueue;

    std::map<std::string, TaskHandlerFunc> m_messageHandlers;
    std::mutex m_messageHandlersMutex;

    // Scheduled tasks by time contains [time_point, ScheduleRecords] pairs.
    // When the time_point is reached, the task is fired and removed. Another pair is added with the next time_point
    // generated from required time matrice
    std::multimap<std::chrono::system_clock::time_point, std::shared_ptr<ScheduleRecord>> m_scheduledTasksByTime;
    bool m_scheduledTaskPushed;
    mutable std::mutex m_scheduledTasksMutex;

    std::thread m_timerThread;
    std::atomic_bool m_runTimerThread;
    std::mutex m_conditionVariableMutex;
    std::condition_variable m_conditionVariable;
    void timer();
    void nextWakeupAndUnlock(std::chrono::system_clock::time_point& timePoint);

    std::map<TaskHandle, std::shared_ptr<ScheduleRecord>> m_scheduledTasksByHandle;

  };

}
