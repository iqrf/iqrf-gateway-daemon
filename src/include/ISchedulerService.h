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
    typedef std::string TaskHandle;
    typedef std::array<std::string, 7> CronType;

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
    virtual std::vector<TaskHandle> getTaskIds(const std::string& clientId) const = 0;

    virtual std::vector<rapidjson::Value *> getTasks(const std::string &clientId, rapidjson::Document::AllocatorType &allocator) const = 0;

    virtual void getTaskDocument(const std::string &clientId, const TaskHandle &taskId, rapidjson::Document &doc) const = 0;

    /// \brief Get a particular tasks for a client
    /// \param [in] clientId client identification
    /// \param [in] hndl task handle identification
    /// \return scheduled tasks
    /// \details
    /// Returns a particular task planned for a client or an empty task if doesn't exists
    virtual const rapidjson::Value* getTask(const std::string& clientId, const TaskHandle& hndl) const = 0;

    virtual const rapidjson::Value* getTaskTimeSpec(const std::string& clientId, const TaskHandle& hndl) const = 0;

    virtual bool isTaskPersistent(const std::string &clientId, const TaskHandle &taskId) const = 0;

    virtual bool isStartupTask(const std::string &clientId, const TaskHandle &taskId) const = 0;

    virtual bool isTaskActive(const std::string &clientId, const TaskHandle &taskId) const = 0;

    virtual TaskHandle scheduleInternalTask(const std::string &clientId, const TaskHandle &taskId, const rapidjson::Value &task, const std::chrono::system_clock::time_point& tp, bool persist, bool enabled) = 0;

    virtual TaskHandle addTask(
      const std::string &clientId,
      const std::string &taskId,
      const std::string &description,
      const rapidjson::Value &task,
      const rapidjson::Value &timeSpec,
      bool persist,
      bool enabled
    ) = 0;

    virtual TaskHandle editTask(
      const std::string &clientId,
      const std::string &taskId,
      const std::string &newTaskId,
      const std::string &description,
      const rapidjson::Value &task,
      const rapidjson::Value &timeSpec,
      bool persist,
      bool enabled
    ) = 0;

    virtual void changeTaskState(const std::string &clientId, const TaskHandle &taskId, bool active) = 0;

    /// \brief Remove all task for client
    /// \param [in] clientId client identification
    /// \details
    /// Scheduler removes all tasks for the client
    virtual void removeAllTasks(const std::string& clientId) = 0;

    /// \brief Remove task for client
    /// \param [in] clientId client identification
    /// \param [in] hndl task handle identification
    /// \details
    /// Scheduler removes a particular tasks for the client
    virtual void removeTask(const std::string& clientId, const TaskHandle &taskId) = 0;

    /// \brief Remove tasks for client
    /// \param [in] clientId client identification
    /// \param [in] hndls task handles identification
    /// \details
    /// Scheduler removes a group of tasks passed in hndls for the client
    virtual void removeTasks(const std::string& clientId, std::vector<TaskHandle> &hndls) = 0;
  };
}
