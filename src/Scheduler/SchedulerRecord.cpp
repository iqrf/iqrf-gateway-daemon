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

#include "SchedulerRecord.h"
#include "rapidjson/pointer.h"
#include "Trace.h"
#include <algorithm>

using namespace std::chrono;

namespace iqrf {

	SchedulerRecord::SchedulerRecord(
		const std::string &clientId,
		const std::string &taskId,
		const rapidjson::Value &task,
		const std::string &startTime,
		bool persist,
		bool enabled
	) : m_clientId(clientId), m_taskId(taskId), m_exactTime(true), m_startTime(startTime), m_persist(persist), m_enabled(enabled)
	{
		TimeConversion::fixTimestamp(m_startTime);
		//m_startTimePoint = daw::date_parsing::parse_iso8601_timestamp(daw::string_view(m_startTime));
		m_startTimePoint = DatetimeParser::parse_to_timepoint(m_startTime);
		init(task);
	}

	SchedulerRecord::SchedulerRecord(
		const std::string &clientId,
		const std::string &taskId,
		const rapidjson::Value &task,
		const std::chrono::system_clock::time_point &startTime,
		bool persist,
		bool enabled
	) : m_clientId(clientId), m_taskId(taskId), m_exactTime(true), m_startTimePoint(startTime), m_persist(persist), m_enabled(enabled)
	{
		init(task);
	}

	SchedulerRecord::SchedulerRecord(
		const std::string &clientId,
		const std::string &taskId,
		const rapidjson::Value &task,
		const std::chrono::seconds &period,
		bool persist,
		bool enabled
	) : m_clientId(clientId), m_taskId(taskId), m_periodic(true), m_period(period), m_persist(persist), m_enabled(enabled)
	{
		if (period.count() <= 0) {
			THROW_EXC_TRC_WAR(std::logic_error, "Period must be at least >= 1sec " << NAME_PAR(period, period.count()))
		}
		init(task);
	}

	SchedulerRecord::SchedulerRecord(
		const std::string &clientId,
		const std::string &taskId,
		const rapidjson::Value &task,
		const std::string &cronString,
		const ISchedulerService::CronType &cronArray,
		bool persist,
		bool enabled
	) : m_clientId(clientId), m_taskId(taskId), m_cron(cronArray), m_cronString(cronString), m_persist(persist), m_enabled(enabled)
	{
		init(task);
	}

	SchedulerRecord::SchedulerRecord(const rapidjson::Value &rec) {
		using namespace rapidjson;

		m_clientId = Pointer("/clientId").Get(rec)->GetString();
		m_taskId = Pointer("/taskId").Get(rec)->GetString();
		const Value *val = Pointer("/description").Get(rec);
		if (val) {
			m_description = val->GetString();
		}
		parseTimeSpec(*Pointer("/timeSpec").Get(rec));
		m_task.CopyFrom(*Pointer("/task").Get(rec), m_task.GetAllocator());
		parseCron();
		val = Pointer("/persist").Get(rec);
		if (val) {
			m_persist = val->GetBool();
		}
		val = Pointer("/enabled").Get(rec);
		if (val) {
			m_enabled = val->GetBool();
		}
	}

	SchedulerRecord::SchedulerRecord(const SchedulerRecord &other) {
		m_clientId = other.m_clientId;
		m_taskId = other.m_taskId;
		m_description = other.m_description;
		m_task.CopyFrom(other.m_task, m_task.GetAllocator());

		m_periodic = other.m_periodic;
		m_period = other.m_period;
		m_exactTime = other.m_exactTime;
		m_startTime = other.m_startTime;
		m_startTimePoint = other.m_startTimePoint;
		m_cron = other.m_cron;
		m_cronString = other.m_cronString;
		m_cronExpr = other.m_cronExpr;
		m_started = other.m_started;

		m_persist = other.m_persist;
		m_enabled = other.m_enabled;
		m_active = other.m_active;

		populateTimeSpec();
	}

	rapidjson::Value SchedulerRecord::serialize(rapidjson::Document::AllocatorType &a) const {
		using namespace rapidjson;
		Value v;
		Pointer("/clientId").Set(v, m_clientId, a);
		Pointer("/taskId").Set(v, m_taskId, a);
		Pointer("/description").Set(v, m_description, a);
		Pointer("/task").Set(v, m_task, a);
		Pointer("/timeSpec").Set(v, m_timeSpec, a);
		Pointer("/persist").Set(v, m_persist, a);
		Pointer("/enabled").Set(v, m_enabled, a);
		return v;
	}

	const std::string& SchedulerRecord::getClientId() const {
		return m_clientId;
	}

	void SchedulerRecord::setClientId(const std::string &clientId) {
		m_clientId = clientId;
	}

	const ISchedulerService::TaskHandle& SchedulerRecord::getTaskId() const {
		return m_taskId;
	}

	void SchedulerRecord::setTaskId(const ISchedulerService::TaskHandle &taskId) {
		m_taskId = taskId;
	}

	const std::string& SchedulerRecord::getDescription() const {
		return m_description;
	}

	void SchedulerRecord::setDescription(const std::string &description) {
		m_description = description;
	}

	const rapidjson::Value& SchedulerRecord::getTask() const {
		return m_task;
	}

	void SchedulerRecord::setTask(const rapidjson::Value &task) {
		m_task.CopyFrom(task, m_task.GetAllocator());
	}

	const rapidjson::Value &SchedulerRecord::getTimeSpec() const {
		return m_timeSpec;
	}

	void SchedulerRecord::setTimeSpec(const rapidjson::Value &timeSpec) {
		m_cron = ISchedulerService::CronType();
		m_cronString = std::string();
		parseTimeSpec(timeSpec);
		parseCron();
	}

	bool SchedulerRecord::isPersistent() const {
		return m_persist;
	}

	void SchedulerRecord::setPersistence(bool persist) {
		m_persist = persist;
	}

	bool SchedulerRecord::isStartupTask() const {
		return m_enabled;
	}

	void SchedulerRecord::setStartupTask(bool enabled) {
		m_enabled = enabled;
	}

	bool SchedulerRecord::isActive() const {
		return m_active;
	}

	void SchedulerRecord::setActive(bool active) {
		m_active = active;
	}

	system_clock::time_point SchedulerRecord::getNext(const std::chrono::system_clock::time_point &actualTimePoint, const std::tm &actualTime) {
		system_clock::time_point tp;
		if (m_exactTime){
			return m_startTimePoint;
		} else if (m_periodic) {
			if (m_started) {
				tp = actualTimePoint + m_period;
			} else {
				tp = std::chrono::system_clock::now();
				m_started = true;
			}
		} else {
			std::time_t now = system_clock::to_time_t(actualTimePoint);
			std::time_t next = cron::cron_next(m_cronExpr, now);
			tp = system_clock::from_time_t(next);
		}
		return tp;
	}

	void SchedulerRecord::getTime(std::chrono::system_clock::time_point &timePoint, std::tm &timeStr) {
		timePoint = system_clock::now();
		time_t tt = system_clock::to_time_t(timePoint);
		std::tm *timeinfo;
		timeinfo = localtime(&tt);
		timeStr = *timeinfo;
	}

	std::string SchedulerRecord::asString(const std::chrono::system_clock::time_point &tp) {
		return TimeConversion::toUTCString(tp);
	}

	///// private methods /////

	void SchedulerRecord::init(const rapidjson::Value &task) {
		TRC_DEBUG("Created: " << PAR(m_taskId));
		m_task.CopyFrom(task, m_task.GetAllocator());
		parseCron();
		populateTimeSpec();
	}

	void SchedulerRecord::parseTimeSpec(const rapidjson::Value &timeSpec) {
		using namespace rapidjson;
		m_timeSpec.CopyFrom(timeSpec, m_timeSpec.GetAllocator());
		const Value *cron = Pointer("/cronTime").Get(timeSpec);
		if (cron->IsArray()) {
			auto it = cron->Begin();
			for (int i = 0; i < 7; i++) {
				m_cron[i] = it->GetString();
				it++;
			}
		} else {
			m_cronString = cron->GetString();
		}
		m_exactTime = Pointer("/exactTime").Get(m_timeSpec)->GetBool();
		m_periodic = Pointer("/periodic").Get(m_timeSpec)->GetBool();
		m_period = std::chrono::seconds(Pointer("/period").Get(m_timeSpec)->GetInt());
		m_startTime = Pointer("/startTime").Get(m_timeSpec)->GetString();
		if (m_startTime.length() > 0) {
			TimeConversion::fixTimestamp(m_startTime);
			m_startTimePoint = daw::date_parsing::parse_iso8601_timestamp(daw::string_view(m_startTime));
		}
	}

	std::string SchedulerRecord::resolveCronAlias(const std::string &alias) {
		auto result = CRON_ALIASES.find(alias);
		if (result == CRON_ALIASES.end()) {
			THROW_EXC_TRC_WAR(std::logic_error, "Unknown or unsupported alias: " << alias);
		}
		return result->second;
	}

	void SchedulerRecord::parseCron() {
		if (m_periodic || m_exactTime) {
			return;
		}
		try {
			if (!m_cronString.empty()) {
				if (m_cronString[0] == '@') {
					m_cronString = resolveCronAlias(m_cronString);
				}
				m_cronExpr = cron::make_cron(m_cronString);
				return;

			}
			std::ostringstream oss;
			for (int i = 0, n = m_cron.size(); i < n; ++i) {
				oss << m_cron[i];
				if ((i+1) != n) {
					oss << ' ';
				}
			}
			m_cronExpr = cron::make_cron(oss.str());
		} catch (const cron::bad_cronexpr &e) {
			THROW_EXC_TRC_WAR(std::logic_error, "Cron expression error: " << e.what());
		}
	}

	void SchedulerRecord::populateTimeSpec() {
		using namespace rapidjson;
		if (m_cronString.length() > 0) {
			Pointer("/cronTime").Set(m_timeSpec, m_cronString);
		} else {
			Pointer("/cronTime/0").Set(m_timeSpec, m_cron[0]);
			Pointer("/cronTime/1").Set(m_timeSpec, m_cron[1]);
			Pointer("/cronTime/2").Set(m_timeSpec, m_cron[2]);
			Pointer("/cronTime/3").Set(m_timeSpec, m_cron[3]);
			Pointer("/cronTime/4").Set(m_timeSpec, m_cron[4]);
			Pointer("/cronTime/5").Set(m_timeSpec, m_cron[5]);
			Pointer("/cronTime/6").Set(m_timeSpec, m_cron[6]);
		}
		Pointer("/exactTime").Set(m_timeSpec, m_exactTime);
		Pointer("/periodic").Set(m_timeSpec, m_periodic);
		Pointer("/period").Set(m_timeSpec, (uint64_t)(m_period.count()));
		if (m_exactTime && m_startTime.length() > 0) {
			Pointer("/startTime").Set(m_timeSpec, SchedulerRecord::asString(m_startTimePoint));
		} else {
			Pointer("/startTime").Set(m_timeSpec, std::string());
		}
	}

	bool SchedulerRecord::verifyTimePattern(int cval, const std::vector<int> &tvalV) const {
		if (tvalV.size() > 0 && tvalV[0] >= 0) {
			for (int tval : tvalV) {
				if (tval == cval) {
					return true;
				}
			}
			return false;
		} else {
			return true;
		}
	}
}
