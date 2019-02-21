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

#include "ScheduleRecord.h"
#include "HexStringCoversion.h"
#include "rapidjson/pointer.h"
#include "Trace.h"
#include <algorithm>

using namespace std::chrono;

namespace iqrf {

  class RandomTaskHandleGenerator {
  private:
    RandomTaskHandleGenerator() {
      //init random seed:
      srand(static_cast<unsigned int>(time(NULL)));
    }
  public:
    static ISchedulerService::TaskHandle getTaskHandle() {
      static RandomTaskHandleGenerator rt;
      int val = rand();
      return (ISchedulerService::TaskHandle)(val ? val : val + 1);
    }
  };

  void ScheduleRecord::init(const rapidjson::Value & task)
  {
    m_taskHandle = RandomTaskHandleGenerator::getTaskHandle();
    TRC_DEBUG("Created: " << PAR(m_taskHandle));
    m_task.CopyFrom(task, m_task.GetAllocator());
    parseCron();
    setTimeSpec();
  }

  //friend of ScheduleRecord
  void shuffleDuplicitHandle(ScheduleRecord& rec)
  {
    rec.shuffleHandle();
  }

  void ScheduleRecord::shuffleHandle()
  {
    ISchedulerService::TaskHandle taskHandleOrig = m_taskHandle;
    m_taskHandle = RandomTaskHandleGenerator::getTaskHandle();
    TRC_DEBUG("Shuffled: " << PAR(m_taskHandle) << PAR(taskHandleOrig));
  }

  //one shot
  ScheduleRecord::ScheduleRecord(const std::string& clientId, const rapidjson::Value & task,
    const std::chrono::system_clock::time_point& startTime, bool persist)
    : m_clientId(clientId)
    , m_exactTime(true)
    , m_startTime(startTime)
    , m_persist(persist)
  {
    init(task);
  }

  //periodic
  ScheduleRecord::ScheduleRecord(const std::string& clientId, const rapidjson::Value & task,
    const std::chrono::seconds& period, const std::chrono::system_clock::time_point& startTime, bool persist)
    : m_clientId(clientId)
    , m_periodic(true)
    , m_period(period)
    , m_startTime(startTime)
    , m_persist(persist)
  {
    if (period.count() <= 0) {
      THROW_EXC_TRC_WAR(std::logic_error, "Period must be at least >= 1sec " << NAME_PAR(period, period.count()))
    }
    init(task);
  }

  //cron
  ScheduleRecord::ScheduleRecord(const std::string& clientId, const rapidjson::Value & task, const ISchedulerService::CronType& cronTime, bool persist)
    : m_clientId(clientId)
    , m_persist(persist)
    , m_cron(cronTime)
  {
    init(task);
  }

  ScheduleRecord::ScheduleRecord(const std::string& clientId, const rapidjson::Value & task, const std::string& cronTime, bool persist)
    : m_clientId(clientId)
    , m_persist(persist)
  {
    std::stringstream strstr(cronTime);
    std::istream_iterator<std::string> it(strstr);
    std::istream_iterator<std::string> end;
    std::move(it, end, m_cron.begin());

    init(task);
  }

  //from persist
  ScheduleRecord::ScheduleRecord(const rapidjson::Value& rec)
  {
    parse(rec);
  }

  ScheduleRecord::ScheduleRecord(const ScheduleRecord& other)
  {
    m_task.CopyFrom(other.m_task, m_task.GetAllocator());
    m_clientId = other.m_clientId;

    m_vsec = other.m_vsec;
    m_vmin = other.m_vmin;
    m_vhour = other.m_vhour;
    m_vmday = other.m_vmday;
    m_vmon = other.m_vmon;
    m_vwday = other.m_vwday;
    m_vyear = other.m_vyear;

    m_exactTime = other.m_exactTime;
    m_periodic = other.m_periodic;
    m_started = other.m_started;
    m_period = other.m_period;
    m_startTime = other.m_startTime;
    m_cron = other.m_cron;

    m_taskHandle = other.m_taskHandle;

    setTimeSpec();
  }

  void ScheduleRecord::setTimeSpec()
  {
    using namespace rapidjson;

    Pointer("/cronTime/0").Set(m_timeSpec, m_cron[0]);
    Pointer("/cronTime/1").Set(m_timeSpec, m_cron[1]);
    Pointer("/cronTime/2").Set(m_timeSpec, m_cron[2]);
    Pointer("/cronTime/3").Set(m_timeSpec, m_cron[3]);
    Pointer("/cronTime/4").Set(m_timeSpec, m_cron[4]);
    Pointer("/cronTime/5").Set(m_timeSpec, m_cron[5]);
    Pointer("/cronTime/6").Set(m_timeSpec, m_cron[6]);
    Pointer("/exactTime").Set(m_timeSpec, m_exactTime);
    Pointer("/periodic").Set(m_timeSpec, m_periodic);
    Pointer("/period").Set(m_timeSpec, m_period.count() * 1000);
    Pointer("/startTime").Set(m_timeSpec, ScheduleRecord::asString(m_startTime));
  }

  void ScheduleRecord::parseTimeSpec(const rapidjson::Value &v)
  {
    using namespace rapidjson;

    m_timeSpec.CopyFrom(v, m_timeSpec.GetAllocator());

    const Value* cron = Pointer("/cronTime").Get(v);
    auto it = cron->Begin();
    for (int i = 0; i < 7; i++) {
      m_cron[i] = it->GetString();
      it++;
    }

    m_exactTime = Pointer("/exactTime").Get(m_timeSpec)->GetBool();
    m_periodic = Pointer("/periodic").Get(m_timeSpec)->GetBool();
    m_period = std::chrono::seconds(Pointer("/period").Get(m_timeSpec)->GetInt());
    m_startTime = parseTimestamp(Pointer("/startTime").Get(m_timeSpec)->GetString());
  }

  const std::map<std::string, std::string> NICKNAMES = {
    {"@reboot", ""},
    {"@yearly", "0 0 0 1 1 * *" },
    {"@annually", "0 0 0 1 1 * *" },
    {"@monthly", "0 0 0 1 * * *" },
    {"@weekly", "0 0 0 * * 0 *" },
    {"@daily", "0 0 0 * * * *" },
    {"@hourly", "0 0 * * * * *" },
    {"@minutely", "0 * * * * * *" }
  };

  void ScheduleRecord::parseCron()
  {
    if (!m_periodic && !m_exactTime) {

      ISchedulerService::CronType tempCron = m_cron;

      //solve nickName
      const std::string& nickName = m_cron[0];
      if (!nickName.empty() && nickName[0] == '@') {
        std::string ts = nickName.substr(0, nickName.find(" ", 0)); //get just beginning string without spaces and other * rubish
        auto found = NICKNAMES.find(ts);
        if (found != NICKNAMES.end()) {
          if (found->second.empty()) { //reboot
            m_exactTime = true;
            m_startTime = std::chrono::system_clock::now();
          }

          std::stringstream strstr(found->second);
          std::istream_iterator<std::string> it(strstr);
          std::istream_iterator<std::string> end;
          std::move(it, end, tempCron.begin());

        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format:" << PAR(nickName));
        }
      }

      if (!m_exactTime) {
        parseItem(tempCron[0], 0, 59, m_vsec);
        parseItem(tempCron[1], 0, 59, m_vmin);
        parseItem(tempCron[2], 0, 23, m_vhour);
        parseItem(tempCron[3], 1, 31, m_vmday);
        parseItem(tempCron[4], 1, 12, m_vmon, -1);
        parseItem(tempCron[5], 0, 6, m_vwday);
        parseItem(tempCron[6], 0, 9999, m_vyear);
      }
    }
  }

  void ScheduleRecord::parse(const rapidjson::Value& rec)
  {
    using namespace rapidjson;

    m_taskHandle = Pointer("/taskId").Get(rec)->GetInt();
    m_clientId = Pointer("/clientId").Get(rec)->GetString();
    parseTimeSpec(*Pointer("/timeSpec").Get(rec));
    m_task.CopyFrom(*Pointer("/task").Get(rec), m_task.GetAllocator());
    parseCron();
  }

  rapidjson::Value ScheduleRecord::serialize(rapidjson::Document::AllocatorType& a) const
  {
    using namespace rapidjson;
    Value v;

    Pointer("/taskId").Set(v, m_taskHandle, a);
    Pointer("/clientId").Set(v, m_clientId, a);
    Pointer("/timeSpec").Set(v, m_timeSpec, a);
    Pointer("/task").Set(v, m_task, a);

    return v;
  }

  int ScheduleRecord::parseItem(const std::string& item, int mnm, int mxm, std::vector<int>& vec, int offset)
  {
    size_t position;
    int val = 0;

    if (item == "*") {
      val = -1;
      vec.push_back(val);
    }

    else if ((position = item.find('/')) != std::string::npos) {
      if (++position > item.size() - 1)
        THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format");
      int divid = std::stoi(item.substr(position));
      if (divid <= 0)
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid value: " << PAR(divid));

      val = mnm % divid;
      val = val == 0 ? mnm : mnm - val + divid;
      while (val < mxm) {
        vec.push_back(val + offset);
        val += divid;
      }
      val = mnm;
    }

    else if ((position = item.find(',')) != std::string::npos) {
      position = 0;
      std::string substr = item;
      while (true) {
        val = std::stoi(substr, &position);
        if (val < mnm || val > mxm)
          THROW_EXC_TRC_WAR(std::logic_error, "Invalid value: " << PAR(val));
        vec.push_back(val + offset);

        if (++position > substr.size() - 1)
          break;
        substr = substr.substr(position);
      }
      val = mnm;
    }

    else {
      val = std::stoi(item);
      if (val < mnm || val > mxm) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid value: " << PAR(val));
      }
      vec.push_back(val + offset);
    }

    std::sort(vec.begin(), vec.end());
    return val;
  }

  system_clock::time_point ScheduleRecord::getNext(const std::chrono::system_clock::time_point& actualTimePoint, const std::tm& actualTime)
  {
    system_clock::time_point tp;

    if (m_exactTime) {
      return m_startTime;
    }
    else if (!m_periodic) {

      //evaluate remaining seconds
      int asec = actualTime.tm_sec;
      int fsec = asec;
      int dsec = 0;

      //find closest valid sec
      if (m_vsec.size() > 0 && m_vsec[0] < 0) {
        fsec = 0; //seconds * use 0 and period is set to 60 sec by default
      }
      else {
        fsec = m_vsec[0];
        for (int sec : m_vsec) { //try to find greater in vector
          if (sec > asec) {
            fsec = sec;
            break;
          }
        }
      }

      dsec = fsec - asec;
      if (fsec <= asec) {
        dsec += 60;
      }

      tp = actualTimePoint + seconds(dsec);

    }
    else {
      if (m_started)
        tp = actualTimePoint + m_period;
      else {
        tp = m_startTime;
        m_started = true;
      }
    }

    return tp;
  }

  bool ScheduleRecord::verifyTimePattern(const std::tm& actualTime) const
  {
    if (!m_periodic && !m_exactTime) {
      if (!verifyTimePattern(actualTime.tm_min, m_vmin)) return false;
      if (!verifyTimePattern(actualTime.tm_hour, m_vhour)) return false;
      if (!verifyTimePattern(actualTime.tm_mday, m_vmday)) return false;
      if (!verifyTimePattern(actualTime.tm_mon, m_vmon)) return false;
      if (!verifyTimePattern(actualTime.tm_wday, m_vwday)) return false;
      if (!verifyTimePattern(actualTime.tm_year, m_vyear)) return false;
    }
    return true;
  }

  bool ScheduleRecord::verifyTimePattern(int cval, const std::vector<int>& tvalV) const
  {
    if (tvalV.size() > 0 && tvalV[0] >= 0) {
      for (int tval : tvalV)
        if (tval == cval)
          return true;
      return false;
    }
    else
      return true;
  }


  std::string ScheduleRecord::asString(const std::chrono::system_clock::time_point& tp)
  {
    return encodeTimestamp(tp);
  }

  void ScheduleRecord::getTime(std::chrono::system_clock::time_point& timePoint, std::tm& timeStr)
  {
    timePoint = system_clock::now();
    time_t tt;
    tt = system_clock::to_time_t(timePoint);
    std::tm* timeinfo;
    timeinfo = localtime(&tt);
    timeStr = *timeinfo;
  }

}