/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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
#include "SchedulerRecord.h"
#include "TaskQueue.h"
#include "ILaunchService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include "rapidjson/schema.h"

#include <string>
#include <chrono>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace iqrf {
	class Scheduler : public ISchedulerService {
	public:
		/**
		 * Constructor
		 */
		Scheduler();

		/**
		 * Destructor
		 */
		virtual ~Scheduler();

		/**
		 * Initializes component
		 * @param props Component configuration
		 */
		void activate(const shape::Properties *props = 0);

		/**
		 * Modifies component properties
		 * @param props Component configuration
		 */
		void modify(const shape::Properties *props);

		/**
		 * Deactivates component
		 */
		void deactivate();

		/**
		 * Registers external task handler function
		 * @param clientId Client ID
		 * @param fun Handler function
		 */
		void registerTaskHandler(const std::string &clientId, TaskHandlerFunc fun) override;

		/**
		 * Unregisters external task handler function
		 * @param clientId Client ID
		 */
		void unregisterTaskHandler(const std::string &clientId) override;

		/**
		 * Returns client task IDs
		 * @param clientId Client ID
		 * @return Vector of tasks belonging to client
		 */
		std::vector<TaskHandle> getTaskIds(const std::string &clientId) const override;

		/**
		 * Returns client tasks in rapidjson value objects
		 * @param clientId Client ID
		 * @param allocator Rapidjson document allocator
		 * @return Vector of rapidjson value task objects
		 */
		std::vector<rapidjson::Value *> getTasks(const std::string &clientId, rapidjson::Document::AllocatorType &allocator) const override;

		/**
		 * Populates a rapidjson document with contents of a client task
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @param doc Rapidjson document
		 */
		void getTaskDocument(const std::string &clientId, const TaskHandle &taskId, rapidjson::Document &doc) const override;

		/**
		 * Returns client task messages
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @return Task messages
		 */
		const rapidjson::Value *getTask(const std::string &clientId, const TaskHandle &taskId) const override;

		/**
		 * Returns task time execution spec
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @return Task execution spec
		 */
		const rapidjson::Value *getTaskTimeSpec(const std::string &clientId, const TaskHandle &taskId) const override;

		/**
		 * Returns task persistence
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @return true if task is persistent, false otherwise
		 */
		bool isTaskPersistent(const std::string &clientId, const TaskHandle &taskId) const override;

		/**
		 * Returns enabled (scheduled on startup) state of client task
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @return true if task is enabled, false otherwise
		 */
		bool isStartupTask(const std::string &clientId, const TaskHandle &taskId) const override;

		/**
		 * Returns active state of client task
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @return true if task is active, false otherwise
		 */
		bool isTaskActive(const std::string &clientId, const TaskHandle &taskId) const override;

		/**
		 * Simplified one-shot task scheduling, for internal use only
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @param task Task messages
		 * @param tp Time point
		 * @param persist Persistent task
		 * @param enabled Enabled (schedule on startup)
		 * @return Task ID
		 */
		TaskHandle scheduleInternalTask(
			const std::string &clientId,
			const std::string &taskId,
			const rapidjson::Value &task,
			const std::chrono::system_clock::time_point& tp,
			bool persist,
			bool enabled
		) override;

		/**
		 * Adds a new task, task is immediately scheduled if enabled
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @param description Task description
		 * @param task Task messages
		 * @param timeSpec Task execution configuration
		 * @param persist Persistent task
		 * @param enabled Enabled (schedule on startup)
		 * @return Task ID
		 */
		TaskHandle addTask(
			const std::string &clientId,
			const std::string &taskId,
			const std::string &description,
			const rapidjson::Value &task,
			const rapidjson::Value &timeSpec,
			bool persist,
			bool enabled
		) override;

		/**
		 * Edits existing client task, and if required, task is rescheduled
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @param newTaskId New task ID
		 * @param description Task description
		 * @param task Task messages
		 * @param timeSpec Task execution configuration
		 * @param persist Persistent task
		 * @param enabled Enabled (schedule on startup)
		 * @return Task ID
		 */
		TaskHandle editTask(
			const std::string &clientId,
			const std::string &taskId,
			const std::string &newTaskId,
			const std::string &description,
			const rapidjson::Value &task,
			const rapidjson::Value &timeSpec,
			bool persist,
			bool enabled
		) override;

		/**
		 * Changes current active state of a client task, if task is already in requested state, no action is taken
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @param active Task state
		 */
		void changeTaskState(const std::string &clientId, const TaskHandle &taskId, bool active) override;

		/**
		 * Removes all client tasks
		 * @param clientId Client ID
		 */
		void removeAllTasks(const std::string &clientId) override;

		/**
		 * Removes client task
		 * @param clientId Client ID
		 * @param taskId Task ID
		 */
		void removeTask(const std::string &clientId, const TaskHandle &taskId) override;

		/**
		 * Removes client tasks
		 * @param clientId Client ID
		 * @param taskIds Vector of task IDs to remove
		 */
		void removeTasks(const std::string &clientId, std::vector<TaskHandle> &taskIds) override;

		/**
		 * Attaches launch service interface
		 * @param iface Launch service interface
		 */
		void attachInterface(shape::ILaunchService *iface);

		/**
		 * Detaches launch service interface
		 * @param iface Launch service interface
		 */
		void detachInterface(shape::ILaunchService *iface);

		/**
		 * Attaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void attachInterface(shape::ITraceService *iface);

		/**
		 * Detaches tracing service interface
		 * @param iface Tracing service interface
		 */
		void detachInterface(shape::ITraceService *iface);
	private:
		/**
		 * Adds a scheduler record and sets active if the task is set to start automatically
		 * If the task is persistent, a task file is created in filesystem
		 * @param record Scheduler record
		 * @param start Start task
		 * @return Task ID
		 */
		TaskHandle addSchedulerTask(std::shared_ptr<SchedulerRecord> &record, bool start = false);

		/**
		 * Removes scheduler record, including active scheduled tasks
		 * If the task is persistent, the task file is deleted
		 * @param record Scheduler record
		 */
		void removeSchedulerTask(std::shared_ptr<SchedulerRecord> &record);

		/**
		 * Creates task file in filesystem from scheduler record, or updates existing file
		 * @param record Scheduler record
		 */
		void writeTaskFile(std::shared_ptr<SchedulerRecord> &record);

		/**
		 * Adds task to map of active tasks
		 * @param record Scheduler record
		 */
		void scheduleTask(std::shared_ptr<SchedulerRecord> &record);

		/**
		 * Removes task from map of active tasks
		 * @param taskId task ID
		 */
		void unscheduleTask(const TaskHandle &taskId);

		/**
		 * Deletes task file from filesystem
		 * @param taskId Task ID
		 */
		void deleteTaskFile(const TaskHandle &taskId);

		/**
		 * Task scheduling and execution worker
		 */
		void worker();

		/**
		 * Calculates next time point for the worker thread to wake up
		 * @param timePoint Current time
		 */
		void getNextWorkerCycleTime(std::chrono::system_clock::time_point &timePoint);

		/**
		 * Wakes up worker thread on-demand
		 */
		void notifyWorker();

		/**
		 * Handles internal scheduled record
		 * @param record Scheduled record
		 * @return int
		 */
		int handleScheduledRecord(const SchedulerRecord &record);

		/**
		 * Loads task files from file system and creates scheduler records
		 * If an old task file is found, it is converted to new task file
		 */
		void loadCache();

		/**
		 * Finds all task files in specified directory and returns set of file paths
		 * @param dir Directory
		 * @return Set of task file paths
		 */
		std::set<std::string> getTaskFiles(const std::string &dir) const;

		/**
		 * Checks if current task ID is used, and generates new one, otherwise returns task ID
		 * @param taskId Current task ID
		 * @return Unique Task ID
		 */
		std::string getTaskHandle(const std::string &taskId);

		/**
		 * Generates unique UUID v4 string
		 * @return UUID string
		 */
		std::string generateTaskId();

		/// Launch service
		shape::ILaunchService *m_iLaunchService = nullptr;
		/// Cache dir
		std::string m_cacheDir;
		/// Path to schema file
		std::string m_schemaFile;
		/// Schema document
		std::shared_ptr<rapidjson::SchemaDocument> m_schema;
		/// Message handler mutex
		std::mutex m_messageHandlersMutex;
		/// Message handlers
		std::map<std::string, TaskHandlerFunc> m_messageHandlers;
		/// Task queue
		TaskQueue<SchedulerRecord> *m_dpaTaskQueue = nullptr;
		/// Scheduled tasks mutex
		mutable std::mutex m_scheduledTasksMutex;
		/// Tasks available
		bool m_scheduledTaskPushed;
		/// Scheduler worker thread
		std::thread m_timerThread;
		/// Worker thread run condition
		std::atomic_bool m_runTimerThread;
		/// Worker thread mutex
		std::mutex m_conditionVariableMutex;
		/// Worker thread condition variable
		std::condition_variable m_conditionVariable;
		/// Map of all tasks
		std::map<TaskHandle, std::shared_ptr<SchedulerRecord>> m_tasksMap;
		/// Map of active, scheduled tasks
		std::multimap<std::chrono::system_clock::time_point, TaskHandle> m_scheduledTasksMap;
		/// Task ID pattern
		const std::string TASK_FILE_PATTERN = "^[0-9a-f]{8}-[0-9a-f]{4}-[4][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}\\.json$";
		/// UUID v4 generator
		boost::uuids::basic_random_generator<boost::mt19937> m_uuidGenerator;
	};
}
