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
#include "rapidjson/pointer.h"
#include "rapidjson/ostreamwrapper.h"
#include "Trace.h"
#include "ShapeDefines.h"
#include <algorithm>
#include <set>

#ifdef SHAPE_PLATFORM_WINDOWS
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

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
    (void)props; //silence -Wunused-parameter

    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "Scheduler instance activate" << std::endl <<
      "******************************"
    );

    using namespace rapidjson;

    m_cacheDir = m_iLaunchService->getCacheDir();
    m_cacheDir += "/scheduler";
    m_schemaFile = m_cacheDir;
    m_schemaFile += "/schema/schema_cache_record.json";
    TRC_INFORMATION("Using cache dir: " << PAR(m_cacheDir));
    
    Document sd;
    std::ifstream ifs(m_schemaFile);
    if (!ifs.is_open()) {
      THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(m_schemaFile));
    }

    IStreamWrapper isw(ifs);
    sd.ParseStream(isw);

    if (sd.HasParseError()) {
      THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " << NAME_PAR(emsg, sd.GetParseError()) <<
        NAME_PAR(eoffset, sd.GetErrorOffset()));
    }

    m_schema = std::shared_ptr<SchemaDocument>(shape_new SchemaDocument(sd));

    loadCache();

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
    using namespace rapidjson;
    const Document& propDoc = props->getAsJson();
    {
      StringBuffer buffer;
      PrettyWriter<StringBuffer> writer(buffer);
      propDoc.Accept(writer);
      std::string cfgStr = buffer.GetString();
      TRC_DEBUG(std::endl << cfgStr);
    }
  }

  void Scheduler::attachInterface(shape::ILaunchService* iface)
  {
    m_iLaunchService = iface;
  }

  void Scheduler::detachInterface(shape::ILaunchService* iface)
  {
    if (m_iLaunchService == iface) {
      m_iLaunchService = nullptr;
    }
  }

  void Scheduler::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void Scheduler::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void Scheduler::loadCache()
  {
    TRC_FUNCTION_ENTER("");
    using namespace rapidjson;
  
    try {
      auto tfiles = getTaskFiles(m_cacheDir);

      for (const auto& fname : tfiles) {
        std::ifstream ifs(fname);
        IStreamWrapper isw(ifs);

        Document d;
        d.ParseStream(isw);
        if (d.HasParseError()) {
          TRC_WARNING("Json parse error: " << NAME_PAR(emsg, d.GetParseError()) <<
            NAME_PAR(eoffset, d.GetErrorOffset()));

          continue; //ignore task 

        }

        SchemaValidator validator(*m_schema);

        if (!d.Accept(validator)) {
          // Input JSON is invalid according to the schema
          StringBuffer sb;
          std::string schema, keyword, document;
          validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
          schema = sb.GetString();
          keyword = validator.GetInvalidSchemaKeyword();
          sb.Clear();
          validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
          document = sb.GetString();
          TRC_WARNING("Invalid " << PAR(schema) << PAR(keyword) << NAME_PAR(message, document));

          continue; //ignore task

        }

        std::shared_ptr<ScheduleRecord> ptr(shape_new ScheduleRecord(d));
        ptr->setPersist(true);

        // lock and copy
        std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
        auto found = m_scheduledTasksByHandle.find(ptr->getTaskHandle());
        if (found != m_scheduledTasksByHandle.end()) {
          TRC_WARNING("Cannot load duplicit: " << NAME_PAR(taskId, ptr->getTaskHandle()))
        }
        else {
          addScheduleRecordUnlocked(ptr);
        }
      }
    }
    catch (std::exception &e) {
      CATCH_EXC_TRC_WAR(std::exception, e, "cannot load scheduler cache")
    }

    TRC_FUNCTION_LEAVE("");
  }

  Scheduler::TaskHandle Scheduler::scheduleTask(const std::string& clientId, const rapidjson::Value & task, const CronType& cronTime, bool persist)
  {
    std::shared_ptr<ScheduleRecord> s = std::shared_ptr<ScheduleRecord>(shape_new ScheduleRecord(clientId, task, cronTime, persist));
    return addScheduleRecord(s);
  }

  Scheduler::TaskHandle Scheduler::scheduleTask(const std::string& clientId, const rapidjson::Value & task, const std::string& cronTime, bool persist)
  {
    std::shared_ptr<ScheduleRecord> s = std::shared_ptr<ScheduleRecord>(shape_new ScheduleRecord(clientId, task, cronTime, persist));
    return addScheduleRecord(s);
  }

  Scheduler::TaskHandle Scheduler::scheduleTaskAt(const std::string& clientId, const rapidjson::Value & task, const std::chrono::system_clock::time_point& tp, bool persist)
  {
    std::shared_ptr<ScheduleRecord> s = std::shared_ptr<ScheduleRecord>(shape_new ScheduleRecord(clientId, task, tp, persist));
    return addScheduleRecord(s);
  }

  Scheduler::TaskHandle Scheduler::scheduleTaskPeriodic(const std::string& clientId, const rapidjson::Value & task, const std::chrono::seconds& sec,
    const std::chrono::system_clock::time_point& tp, bool persist)
  {
    std::shared_ptr<ScheduleRecord> s = std::shared_ptr<ScheduleRecord>(shape_new ScheduleRecord(clientId, task, sec, tp, persist));
    return addScheduleRecord(s);
  }

  int Scheduler::handleScheduledRecord(const ScheduleRecord& record)
  {
    //TRC_DEBUG("==================================" << std::endl <<
    //  "Scheduled msg: " << std::endl << FORM_HEX(record.getTask().data(), record.getTask().size()));

    {
      std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
      try {
        auto found = m_messageHandlers.find(record.getClientId());
        if (found != m_messageHandlers.end()) {
          //TRC_DEBUG(NAME_PAR(Task, record.getTask()) << " has been passed to: " << NAME_PAR(ClinetId, record.getClientId()));
          found->second(record.getTask());
        }
        else {
          TRC_DEBUG("Unregistered client: " << PAR(record.getClientId()));
        }
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "untreated handler exception");
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

    if (record->isPersist()) {
      using namespace rapidjson;

      std::ostringstream os;
      os << m_cacheDir << '/' << record->getTaskHandle() << ".json";
      std::string fname = os.str();

      std::ifstream ifs(fname);
      if (ifs.good()) {
        TRC_WARNING("File already exists: " << PAR(fname));
      }
      else {
        Document d;
        auto v = record->serialize(d.GetAllocator());
        d.Swap(v);
        std::ofstream ofs(fname);
        OStreamWrapper osw(ofs);
        PrettyWriter<OStreamWrapper> writer(osw);
        d.Accept(writer);
      }
    }

    addScheduleRecordUnlocked(record);

    // notify timer thread
    std::unique_lock<std::mutex> lckn(m_conditionVariableMutex);
    m_scheduledTaskPushed = true;
    m_conditionVariable.notify_one();

    return record->getTaskHandle();
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
    
    if (record->isPersist()) {
      std::ostringstream os;
      os << m_cacheDir << '/' << record->getTaskHandle() << ".json";
      std::string fname = os.str();
      std::remove(fname.c_str());
    }

    m_scheduledTasksByHandle.erase(handle);
  }

  void Scheduler::removeScheduleRecord(std::shared_ptr<ScheduleRecord>& record)
  {
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    removeScheduleRecordUnlocked(record);
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
        //auto diff = begin->first.time_since_epoch().count() - timePoint.time_since_epoch().count();

        if (begin->first < timePoint) {

          // erase fired
          m_scheduledTasksByTime.erase(begin);

          // get and schedule next
          system_clock::time_point nextTimePoint = record->getNext(timePoint, timeStr);
          if (nextTimePoint >= timePoint) {
            m_scheduledTasksByTime.insert(std::make_pair(nextTimePoint, record));
          }
          else {
            //expired one shot task - remove from m_scheduledTasksByHandle
            removeScheduleRecordUnlocked(record);
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

  void Scheduler::registerTaskHandler(const std::string& clientId, TaskHandlerFunc fun)
  {
    std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
    //TODO check success
    m_messageHandlers.insert(make_pair(clientId, fun));
  }

  void Scheduler::unregisterTaskHandler(const std::string& clientId)
  {
    std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
    m_messageHandlers.erase(clientId);
  }

  std::vector<ISchedulerService::TaskHandle> Scheduler::getMyTasks(const std::string& clientId) const
  {
    std::vector<TaskHandle> retval;
    // lock and copy
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    for (auto & task : m_scheduledTasksByTime) {
      if (task.second->getClientId() == clientId) {
        retval.push_back(task.second->getTaskHandle());
      }
    }
    return retval;
  }

  const rapidjson::Value * Scheduler::getMyTask(const std::string& clientId, const TaskHandle& hndl) const
  {
    const rapidjson::Value * retval = nullptr;
    // lock and copy
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    auto found = m_scheduledTasksByHandle.find(hndl);
    if (found != m_scheduledTasksByHandle.end() && clientId == found->second->getClientId()) {
      retval = &found->second->getTask();
      //const rapidjson::Value& vvv = found->second->getTask();
    }
    return retval;
  }

  const rapidjson::Value * Scheduler::getMyTaskTimeSpec(const std::string& clientId, const TaskHandle& hndl) const
  {
    const rapidjson::Value * retval = nullptr;
    // lock and copy
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    auto found = m_scheduledTasksByHandle.find(hndl);
    if (found != m_scheduledTasksByHandle.end() && clientId == found->second->getClientId())
      retval = &found->second->getTimeSpec();
    return retval;
  }

  bool Scheduler::isPersist(const std::string& clientId, const TaskHandle& hndl) const
  {
    bool retval = false;
    // lock and copy
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    auto found = m_scheduledTasksByHandle.find(hndl);
    if (found != m_scheduledTasksByHandle.end() && clientId == found->second->getClientId())
      retval = found->second->isPersist();
    return retval;
  }

  void Scheduler::removeAllMyTasks(const std::string& clientId)
  {
    // lock and remove
    std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
    for (auto it = m_scheduledTasksByTime.begin(); it != m_scheduledTasksByTime.end(); ) {
      if (it->second->getClientId() == clientId) {
        //remove persist file
        if (it->second->isPersist()) {
          std::ostringstream os;
          os << m_cacheDir << '/' << it->second->getTaskHandle() << ".json";
          std::string fname = os.str();
          std::remove(fname.c_str());
        }
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

#ifdef SHAPE_PLATFORM_WINDOWS
  std::set<std::string> Scheduler::getTaskFiles(const std::string& dir) const
  {
    WIN32_FIND_DATA fid;
    HANDLE found = INVALID_HANDLE_VALUE;

    std::set<std::string>  fileSet;
    std::string sdirect(dir);
    sdirect.append("/*.json");

    found = FindFirstFile(sdirect.c_str(), &fid);

    if (INVALID_HANDLE_VALUE == found) {
      TRC_INFORMATION("Directory does not exist or empty Scheduler cache: " << PAR(sdirect));
    }

    do {
      if (fid.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        continue; //skip a directory
      std::string fil(dir);
      fil.append("/");
      fil.append(fid.cFileName);
      fileSet.insert(fil);
    } while (FindNextFile(found, &fid) != 0);

    FindClose(found);
    return fileSet;
  }

#else
  std::set<std::string> Scheduler::getTaskFiles(const std::string& dirStr) const
  {
    std::set<std::string> fileSet;
    std::string jsonExt = "json";

    DIR *dir;
    class dirent *ent;
    class stat st;

    dir = opendir(dirStr.c_str());
    if (dir == nullptr) {
      TRC_INFORMATION("Directory does not exist or empty Scheduler cache: " << PAR(dirStr));
    }
    else {
      while ((ent = readdir(dir)) != NULL) {
        const std::string file_name = ent->d_name;
        const std::string full_file_name(dirStr + "/" + file_name);

        if (file_name[0] == '.')
          continue;

        if (stat(full_file_name.c_str(), &st) == -1)
          continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;

        if (is_directory)
          continue;

        //keep just *.json
        size_t i = full_file_name.rfind('.', full_file_name.length());
        if (i != std::string::npos && jsonExt == full_file_name.substr(i + 1, full_file_name.length() - i)) {
          fileSet.insert(full_file_name);
        }
        else {
          continue;
        }
      }
      closedir(dir);
 
    }

    return fileSet;
  }

#endif

}
