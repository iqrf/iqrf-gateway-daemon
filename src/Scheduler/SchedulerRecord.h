/**
 * Copyright 2015-2023 IQRF Tech s.r.o.
 * Copyright 2019-2023 MICRORISC s.r.o.
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
#include "DatetimeParser.h"
#include "TimeConversion.h"
#include "croncpp.h"

#include <string>
#include <chrono>
#include <array>
#include <numeric>

/// iqrf namespace
namespace iqrf {
	/// \class SchedulerRecord
	/// \brief Auxiliary class to handle scheduler tasks.
	class SchedulerRecord {
	public:
		/// Delete base constructor
		SchedulerRecord() = delete;

		/**
		 * One-shot or cron string task constructor
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @param task Task messages
		 * @param cron Cron time
		 * @param timeString Task execution time string or cronstring
		 * @param persist Persistent task
		 * @param enabled Start task automatically
		 */
		SchedulerRecord(
			const std::string &clientId,
			const std::string &taskId,
			const rapidjson::Value &task,
			bool cron,
			const std::string &timeString,
			bool persist,
			bool enabled
		);

		/**
		 * One-shot time point task constructor
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @param task Task messages
		 * @param startTime Task execution timestamp
		 * @param persist Persistent task
		 * @param enabled Start task automatically
		 */
		SchedulerRecord(
			const std::string &clientId,
			const std::string &taskId,
			const rapidjson::Value &task,
			const std::chrono::system_clock::time_point &startTime,
			bool persist,
			bool enabled
		);

		/**
		 * Periodic task constructor
		 * @param clientId Client ID
		 * @param taskId Task ID
		 * @param task Task messages
		 * @param period Task execution period
		 * @param persist Persistent task
		 * @param enabled Start task automatically
		 */
		SchedulerRecord(
			const std::string &clientId,
			const std::string &taskId,
			const rapidjson::Value &task,
			const std::chrono::seconds &period,
			bool persist,
			bool enabled
		);

		/**
		 * Rapidjson document constructor
		 * @param rec Rapidjson document
		 */
		SchedulerRecord(const rapidjson::Value &rec);

		/**
		 * Copy constructor
		 * @param other Other record
		 */
		SchedulerRecord(const SchedulerRecord &other);

		/**
		 * Serializes scheduler record to rapidjson value
		 * @param allocator Rapidjson document allocator
		 * @return Rapidjson-serialized value
		 */
		rapidjson::Value serialize(rapidjson::Document::AllocatorType &allocator) const;

		/**
		 * Returns client ID
		 * @return Client ID
		 */
		const std::string& getClientId() const;

		/**
		 * Sets client ID
		 * @param clientId Client ID
		 */
		void setClientId(const std::string &clientId);

		/**
		 * Returns task ID
		 * @return Task ID
		 */
		const ISchedulerService::TaskHandle& getTaskId() const;

		/**
		 * Sets task ID
		 * @param taskId Task ID
		 */
		void setTaskId(const ISchedulerService::TaskHandle &taskId);

		/**
		 * Returns task description
		 * @return Task description
		 */
		const std::string& getDescription() const;

		/**
		 * Sets task description
		 * @param description Task description
		 */
		void setDescription(const std::string &description);

		/**
		 * Returns task messages
		 * @return Task messages
		 */
		const rapidjson::Value& getTask() const;

		/**
		 * Sets task messages
		 * @param task Task messages
		 */
		void setTask(const rapidjson::Value &task);

		/**
		 * Returns task execution timespec
		 * @return Task execution timespec
		 */
		const rapidjson::Value &getTimeSpec() const;

		/**
		 * Sets task execution timespec
		 * @param timeSpec Task exection timespec
		 */
		void setTimeSpec(const rapidjson::Value &timeSpec);

		/**
		 * Returns task persistence
		 * @return true if task is persistent, false otherwise
		 */
		bool isPersistent() const;

		/**
		 * @brief Sets task persistence
		 * @param persist Task persistence
		 */
		void setPersistence(bool persist);

		/**
		 * Returns task startup
		 * @return true if task starts automatically, false otherwise
		 */
		bool isStartupTask() const;

		/**
		 * Sets task startup
		 * @param enabled Task startup
		 */
		void setStartupTask(bool enabled);

		/**
		 * Returns task active state
		 * @return true if task is scheduled, false otherwise
		 */
		bool isActive() const;

		/**
		 * Sets task active state
		 * @param active Task active state
		 */
		void setActive(bool active);

		/**
		 * Calculates the nearest task execution time
		 * @param actualTimePoint Time point
		 * @param actualTime Broken-down time
		 * @return Task execution time point
		 */
		std::chrono::system_clock::time_point getNext(const std::chrono::system_clock::time_point &actualTimePoint, const std::tm &actualTime);

		/**
		 * Populates time point and broken-down time structure with current time data
		 * @param timePoint Time point
		 * @param timeStr Broken-down time
		 */
		static void getTime(std::chrono::system_clock::time_point &timePoint, std::tm &timeStr);

		/**
		 * Converts time point to string representation
		 * @param tp Time point
		 * @return String representation of time point
		 */
		static std::string asString(const std::chrono::system_clock::time_point &tp);
	private:
		/**
		 * Intializes scheduler record
		 * @param task Task message value
		 */
		void init(const rapidjson::Value &task);

		/**
		 * Parses time specification rapidjson value into task execution time units
		 * @param timeSpec Time specification rapidjson value
		 */
		void parseTimeSpec(const rapidjson::Value &timeSpec);

		/**
		 * Parses cron string into task execution time units
		 */
		void parseCron();

		/**
		 * Attempts to resolve cron alias
		 * @param alias Cron alias
		 * @return Resolved cron alias
		 */
		std::string resolveCronAlias(const std::string &alias);

		/**
		 * Populates internal time specification document with execution time data
		 */
		void populateTimeSpec();

		/**
		 * Checks if current time unit value is an execution time value
		 * @param cval Current time unit value
		 * @param tvalV Execution time unit values
		 * @return true if current time is execution time, false otherwise
		 */
		bool verifyTimePattern(int cval, const std::vector<int> &tvalV) const;

		/// Map of cron aliases and equivalent cron expressions
		const std::map<std::string, std::string> CRON_ALIASES = {
			{"@yearly", "0 0 0 1 1 * *"},
			{"@annually", "0 0 0 1 1 * *"},
			{"@monthly", "0 0 0 1 * * *"},
			{"@weekly", "0 0 0 * * 0 *"},
			{"@daily", "0 0 0 * * * *"},
			{"@hourly", "0 0 * * * * *"},
			{"@minutely", "0 * * * * * *"}
		};
		/// Client ID
		std::string m_clientId;
		/// Task ID
		ISchedulerService::TaskHandle m_taskId;
		/// Task description
		std::string m_description;
		/// Task messages
		rapidjson::Document m_task;
		/// Task execution time
		rapidjson::Document m_timeSpec;
		/// Periodic task
		bool m_periodic = false;
		/// Execution period
		std::chrono::seconds m_period = std::chrono::seconds(0);
		/// One-shot task
		bool m_exactTime = false;
		/// Start timestamp
		std::string m_startTime;
		/// Execution time point
		std::chrono::system_clock::time_point m_startTimePoint;
		/// Cron string
		std::string m_cronString;
		/// Parsed cron expression
		cron::cronexpr m_cronExpr;
		/// Persistent task
		bool m_persist = false;
		/// Start task automatically
		bool m_enabled = false;
		/// Is task active
		bool m_active = false;
		/// Started
		bool m_started = false;
	};
}
