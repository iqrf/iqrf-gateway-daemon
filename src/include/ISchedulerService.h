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

#include "ShapeDefines.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <string>
#include <functional>
#include <vector>
#include <chrono>
#include <array>

#ifdef ISchedulerService_EXPORTS
#define ISchedulerService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define ISchedulerService_DECLSPEC SHAPE_ABI_IMPORT
#endif

/// \class ISchedulerService
/// \brief ISchedulerService interface
/// \details
/// Provides interface for planning task fired at proper time
namespace iqrf {
  class ISchedulerService_DECLSPEC ISchedulerService
  {
  public:
    /// Task handle is task identification
    typedef int32_t TaskHandle;

    /// Invalid task handle
    static const TaskHandle TASK_HANDLE_INVALID = 0;

    /// Task to be processed handler functional type
    typedef std::function<void(const rapidjson::Value &)> TaskHandlerFunc;

    virtual ~ISchedulerService() {};

    /// \brief Register task handler
    /// \param [in] clientId client identification registering handler function
    /// \param [in] fun handler function
    /// \details
    /// Whenever the scheduler evaluate a task to be handled it is passed to the handler function.
    /// The only tasks planned for particular clientId are delivered.
    /// Repeated registration with the same client identification replaces previously registered handler
    virtual void registerTaskHandler(const std::string& clientId, TaskHandlerFunc fun) = 0;

    /// \brief Unregister task handler
    /// \param [in] clientId client identification
    /// \details
    /// If the handler is not required anymore, it is possible to unregister via this method.
    virtual void unregisterTaskHandler(const std::string& clientId) = 0;

    /// \brief Get scheduled tasks for a client
    /// \param [in] clientId client identification
    /// \return scheduled tasks
    /// \details
    /// Returns all pending scheduled tasks for the client
    //virtual std::vector<const rapidjson::Value *> getMyTasks(const std::string& clientId) const = 0;
    virtual std::vector<TaskHandle> getMyTasks(const std::string& clientId) const = 0;

    /// \brief Get a particular tasks for a client
    /// \param [in] clientId client identification
    /// \param [in] hndl task handle identification
    /// \return scheduled tasks
    /// \details
    /// Returns a particular task planned for a client or an empty task if doesn't exists
    virtual const rapidjson::Value * getMyTask(const std::string& clientId, const TaskHandle& hndl) const = 0;

    virtual const rapidjson::Value * getMyTaskTimeSpec(const std::string& clientId, const TaskHandle& hndl) const = 0;

    // if exists and is persist return true
    virtual bool isPersist(const std::string& clientId, const TaskHandle& hndl) const = 0;

    typedef std::array<std::string, 7> CronType;
    virtual TaskHandle scheduleTask(const std::string& clientId, const rapidjson::Value & task, const CronType& cronTime, bool persist = false) = 0;
    virtual TaskHandle scheduleTask(const std::string& clientId, const rapidjson::Value & task, const std::string& cronTime, bool persist = false) = 0;

    /// \brief Schedule task at time point
    /// \param [in] clientId client identification
    /// \param [in] task planned task
    /// \param [in] tp time point
    /// \return scheduled task handle
    /// \details
    /// Schedules task at exact time point. When the time point is reached the task is passed to its handler and released
    /// Use it for one shot tasks
    virtual TaskHandle scheduleTaskAt(const std::string& clientId, const rapidjson::Value & task, const std::chrono::system_clock::time_point& tp, bool persist = false) = 0;

    /// \brief Schedule periodic task
    /// \param [in] clientId client identification
    /// \param [in] task planned task
    /// \param [in] sec period in seconds
    /// \param [in] tp time point of delayed start
    /// \return scheduled task handle
    /// \details
    /// Schedules periodic task. It is started immediatelly by default, the first shot after one period.
    /// If the start shall be delayed use appropriate time point of start
    virtual TaskHandle scheduleTaskPeriodic(const std::string& clientId, const rapidjson::Value &, const std::chrono::seconds& sec,
      const std::chrono::system_clock::time_point& tp = std::chrono::system_clock::now(), bool persist = false) = 0;

    /// \brief Remove all task for client
    /// \param [in] clientId client identification
    /// \details
    /// Scheduler removes all tasks for the client
    virtual void removeAllMyTasks(const std::string& clientId) = 0;

    /// \brief Remove task for client
    /// \param [in] clientId client identification
    /// \param [in] hndl task handle identification
    /// \details
    /// Scheduler removes a particular tasks for the client
    virtual void removeTask(const std::string& clientId, TaskHandle hndl) = 0;

    /// \brief Remove tasks for client
    /// \param [in] clientId client identification
    /// \param [in] hndls task handles identification
    /// \details
    /// Scheduler removes a group of tasks passed in hndls for the client
    virtual void removeTasks(const std::string& clientId, std::vector<TaskHandle> hndls) = 0;
  };
}
