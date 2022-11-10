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
		Scheduler();
		virtual ~Scheduler();

		void activate(const shape::Properties *props = 0);
		void deactivate();
		void modify(const shape::Properties *props);

		void registerTaskHandler(const std::string &clientId, TaskHandlerFunc fun) override;
		void unregisterTaskHandler(const std::string &clientId) override;

		std::vector<TaskHandle> getTaskIds(const std::string &clientId) const override;
		std::vector<rapidjson::Value *> getTasks(const std::string &clientId, rapidjson::Document::AllocatorType &allocator) const override;
		void getTasks(const std::string &clientId) const;
		void getTaskDocument(const std::string &clientId, const TaskHandle &taskId, rapidjson::Document &doc) const override;
		const rapidjson::Value *getTask(const std::string &clientId, const TaskHandle &taskId) const override;
		const rapidjson::Value *getTaskTimeSpec(const std::string &clientId, const TaskHandle &taskId) const override;
		bool isTaskPersistent(const std::string &clientId, const TaskHandle &taskId) const override;
		bool isStartupTask(const std::string &clientId, const TaskHandle &taskId) const override;
		bool isTaskActive(const std::string &clientId, const TaskHandle &taskId) const override;
		TaskHandle scheduleInternalTask(
			const std::string &clientId,
			const std::string &taskId,
			const rapidjson::Value &task,
			const std::chrono::system_clock::time_point& tp,
			bool persist,
			bool enabled
		) override;
		TaskHandle addTask(
			const std::string &clientId,
			const std::string &taskId,
			const std::string &description,
			const rapidjson::Value &task,
			const rapidjson::Value &timeSpec,
			bool persist,
			bool enabled) override;
		TaskHandle editTask(
			const std::string &clientId,
			const std::string &taskId,
			const std::string &newTaskId,
			const std::string &description,
			const rapidjson::Value &task,
			const rapidjson::Value &timeSpec,
			bool persist,
			bool enabled) override;
		void changeTaskState(const std::string &clientId, const TaskHandle &taskId, bool active) override;
		void removeAllTasks(const std::string &clientId) override;
		void removeTask(const std::string &clientId, const TaskHandle &taskId) override;
		void removeTasks(const std::string &clientId, std::vector<TaskHandle> &taskIds) override;

		void attachInterface(shape::ILaunchService *iface);
		void detachInterface(shape::ILaunchService *iface);

		void attachInterface(shape::ITraceService *iface);
		void detachInterface(shape::ITraceService *iface);

	private:
		void loadCache();
		int handleScheduledRecord(const SchedulerRecord &record);
		TaskHandle addSchedulerTask(std::shared_ptr<SchedulerRecord> &record);
		void createTaskFile(std::shared_ptr<SchedulerRecord> &record);
		void scheduleTask(std::shared_ptr<SchedulerRecord> &record);
		void unscheduleTask(const TaskHandle &taskId);
		void deleteTaskFile(const TaskHandle &taskId);
		void removeSchedulerTask(std::shared_ptr<SchedulerRecord> &record);
		void worker();
		void getNextWorkerCycleTime(std::chrono::system_clock::time_point &timePoint);
		std::set<std::string> getTaskFiles(const std::string &dir) const;
		std::string getTaskHandle(const std::string &taskId);
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
