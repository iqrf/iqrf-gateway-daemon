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
#pragma once

#include "JsonUtils.h"
#include "rapidjson/rapidjson.h"
#include "TaskQueue.h"
#include "ISchedulerService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"

#include <string>
#include <chrono>
#include <array>
#include <numeric>

namespace iqrf {
  /// \class SchedulerRecord
  /// \brief Auxiliary class to handle scheduled tasks.
  class ScheduleRecord {
  public:
    ScheduleRecord() = delete;

    //one shot
    ScheduleRecord(const std::string& clientId, const rapidjson::Value & task, const std::chrono::system_clock::time_point& startTime, bool persist);

    //periodic
    ScheduleRecord(const std::string& clientId, const rapidjson::Value & task, const std::chrono::seconds& period,
      const std::chrono::system_clock::time_point& startTime, bool persist);

    //cron
    //These special time specification "nicknames" which replace initial time and date fields,
    //and are prefixed with the '@' character, are supported :
    //  @reboot : Run once after reboot.
    //  @yearly : Run once a year, ie.  "0 0 0 0 1 1 *".
    //  @annually : Run once a year, ie.  "0 0 0 0 1 1 *".
    //  @monthly : Run once a month, ie. "0 0 0 0 1 * *".
    //  @weekly : Run once a week, ie.  "0 0 0 * * 0 *".
    //  @daily : Run once a day, ie.   "0 0 0 * * * *".
    //  @hourly : Run once an hour, ie. "0 0 * * * * *".
    //  @minutely : Run once a minute, ie. "0 * * * * * *".
    ScheduleRecord(const std::string& clientId, const rapidjson::Value & task, const ISchedulerService::CronType& cronTime, bool persist);
    ScheduleRecord(const std::string& clientId, const rapidjson::Value & task, const std::string& cronTime, bool persist);

    //from persist file
    ScheduleRecord(const rapidjson::Value& rec);

    //copy
    ScheduleRecord(const ScheduleRecord& other);

    ISchedulerService::TaskHandle getTaskHandle() const { return m_taskHandle; }
    std::chrono::system_clock::time_point getNext(const std::chrono::system_clock::time_point& actualTimePoint, const std::tm& actualTime);
    bool verifyTimePattern(const std::tm& actualTime) const;
    const rapidjson::Value& getTask() const { return m_task; }
    const std::string& getClientId() const { return m_clientId; }
    const rapidjson::Value& getTimeSpec() const { return m_timeSpec; }
    bool isPersist() const { return m_persist; }
    void setPersist(bool persist) { m_persist = persist; }

    static std::string asString(const std::chrono::system_clock::time_point& tp);
    static void getTime(std::chrono::system_clock::time_point& timePoint, std::tm& timeStr);

    void parse(const rapidjson::Value& rec);
    rapidjson::Value serialize(rapidjson::Document::AllocatorType& a) const;

  private:
    void parseCron();
    //Change handle it if duplicit detected by Scheduler
    void shuffleHandle(); //change handle it if duplicit exists
    //The only method can do it
    friend void shuffleDuplicitHandle(ScheduleRecord& rec);
    void init(const rapidjson::Value & task);
    int parseItem(const std::string& item, int min, int max, std::vector<int>& vec, int offset = 0);
    void setTimeSpec();
    void parseTimeSpec(const rapidjson::Value &v);

    bool verifyTimePattern(int cval, const std::vector<int>& tvalV) const;
    rapidjson::Document m_task;
    std::string m_clientId;

    //multi record
    std::vector<int> m_vsec;
    std::vector<int> m_vmin;
    std::vector<int> m_vhour;
    std::vector<int> m_vmday;
    std::vector<int> m_vmon;
    std::vector<int> m_vwday;
    std::vector<int> m_vyear;

    //explicit timing
    bool m_exactTime = false;
    bool m_periodic = false;
    bool m_started = false;
    std::chrono::seconds m_period = std::chrono::seconds(0);
    std::chrono::system_clock::time_point m_startTime;
    bool m_persist = false;
    ISchedulerService::TaskHandle m_taskHandle;
    rapidjson::Document m_timeSpec;
    ISchedulerService::CronType m_cron;
    std::string m_cronString;
  };
}
