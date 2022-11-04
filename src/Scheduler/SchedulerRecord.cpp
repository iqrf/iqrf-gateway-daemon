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
		const std::chrono::system_clock::time_point &startTime,
		bool persist,
		bool autoStart
	) : m_clientId(clientId), m_taskId(taskId), m_exactTime(true), m_startTime(startTime), m_persist(persist), m_autoStart(autoStart)
	{
		init(task);
	}

	SchedulerRecord::SchedulerRecord(
		const std::string &clientId,
		const std::string &taskId,
		const rapidjson::Value &task,
		const std::chrono::seconds &period,
		const std::chrono::system_clock::time_point &startTime,
		bool persist,
		bool autoStart
	) : m_clientId(clientId), m_taskId(taskId), m_periodic(true), m_period(period), m_startTime(startTime), m_persist(persist), m_autoStart(autoStart)
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
		const ISchedulerService::CronType &cronTime,
		bool persist,
		bool autoStart
	) : m_clientId(clientId), m_taskId(taskId), m_cron(cronTime), m_persist(persist), m_autoStart(autoStart)
	{
		init(task);
	}

	SchedulerRecord::SchedulerRecord(
		const std::string &clientId,
		const std::string &taskId,
		const rapidjson::Value &task,
		const std::string &cronTime,
		bool persist,
		bool autoStart
	) : m_clientId(clientId), m_taskId(taskId), m_cronString(cronTime), m_persist(persist), m_autoStart(autoStart)
	{
		init(task);
	}

	SchedulerRecord::SchedulerRecord(const rapidjson::Value &rec) {
		using namespace rapidjson;

		m_clientId = Pointer("/clientId").Get(rec)->GetString();
		m_taskId = Pointer("/taskId").Get(rec)->GetString();
		m_description = Pointer("/description").Get(rec)->GetString();
		parseTimeSpec(*Pointer("/timeSpec").Get(rec));
		m_task.CopyFrom(*Pointer("/task").Get(rec), m_task.GetAllocator());
		parseCron();
		const Value *val = Pointer("/persist").Get(rec);
		if (val) {
			m_persist = val->GetBool();
		}
		val = Pointer("/autoStart").Get(rec);
		if (val) {
			m_autoStart = val->GetBool();
		}
	}

	SchedulerRecord::SchedulerRecord(const SchedulerRecord &other) {
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

		m_taskId = other.m_taskId;
		m_description = other.m_description;
		m_persist = other.m_persist;
		m_autoStart = other.m_autoStart;

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
		Pointer("/autoStart").Set(v, m_autoStart, a);
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
		return m_autoStart;
	}

	void SchedulerRecord::setStartupTask(bool autoStart) {
		m_autoStart = autoStart;
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
			return m_startTime;
		} else if (m_periodic) {
			if (m_started) {
				tp = actualTimePoint + m_period;
			} else {
				tp = m_startTime;
				m_started = true;
			}
		} else {
			//std::time_t now = std::time(0);
			std::time_t now = system_clock::to_time_t(actualTimePoint);
			std::time_t next = cron::cron_next(m_cronExpr, now);
			tp = system_clock::from_time_t(next);
			/*
			//evaluate remaining seconds
			int asec = actualTime.tm_sec;
			int fsec = asec;
			int dsec = 0;
			// find closest valid sec
			if (m_vsec.size() > 0 && m_vsec[0] < 0) {
				fsec = 0; // seconds * use 0 and period is set to 60 sec by default
			} else {
				fsec = m_vsec._Find_first();
				for (int i = asec + 1, n = m_vsec.size(); i < n; ++i) {
					if (m_vsec.test(i)) {
						fsec = i;
						break;
					}
				}
			}
			dsec = fsec - asec;
			if (fsec <= asec) {
				dsec += 60;
			}
			tp = actualTimePoint + seconds(dsec);
			*/
		}
		return tp;
	}

	bool SchedulerRecord::isExecutionTime(const std::tm &actualTime) const {
		if (!m_periodic && !m_exactTime) {
			if (!m_vmin.test(actualTime.tm_min)) {
				return false;
			}
			if (!m_vhour.test(actualTime.tm_hour)) {
				return false;
			}
			if (!m_vmday.test(actualTime.tm_mday - 1)) {
				return false;
			}
			if (!m_vmon.test(actualTime.tm_mon)) {
				return false;
			}
			if (!m_vwday.test(actualTime.tm_wday)) {
				return false;
			}
			if (!m_vyear.test(actualTime.tm_year - 70)) {
				return false;
			}
		}
		return true;
	}

	void SchedulerRecord::getTime(std::chrono::system_clock::time_point &timePoint, std::tm &timeStr) {
		timePoint = system_clock::now();
		time_t tt;
		tt = system_clock::to_time_t(timePoint);
		std::tm *timeinfo;
		timeinfo = localtime(&tt);
		timeStr = *timeinfo;
	}

	std::string SchedulerRecord::asString(const std::chrono::system_clock::time_point &tp) {
		return TimeConversion::encodeTimestamp(tp);
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
		m_startTime = TimeConversion::parseTimestamp(Pointer("/startTime").Get(m_timeSpec)->GetString());
	}

	void SchedulerRecord::parseCron() {
		if (!m_periodic && !m_exactTime) {
			ISchedulerService::CronType tempCron = m_cron;
			if (!m_cronString.empty() && m_cronString[0] == '@') {
				std::string ts = m_cronString.substr(0, m_cronString.find(" ", 0)); // get just beginning string without spaces and other * rubish
				auto found = CRON_ALIASES.find(ts);
				if (found != CRON_ALIASES.end()) {
					if (found->second.empty()) {
						m_exactTime = true;
						m_startTime = std::chrono::system_clock::now();
					}
					std::stringstream strstr(found->second);
					std::istream_iterator<std::string> it(strstr);
					std::istream_iterator<std::string> end;
					std::move(it, end, tempCron.begin());
				} else {
					THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format:" << PAR(m_cronString));
				}
			}
			if (!m_exactTime) {
				/*parseItem(tempCron[0], 0, 59, m_vsec);
				parseItem(tempCron[1], 0, 59, m_vmin);
				parseItem(tempCron[2], 0, 23, m_vhour);
				parseItem(tempCron[3], 1, 31, m_vmday, -1);
				parseItem(tempCron[4], 1, 12, m_vmon, -1);
				parseItem(tempCron[5], 0, 6, m_vwday);
				parseItem(tempCron[6], 1970, 2099, m_vyear, -1970);*/
				m_cronExpr = cron::make_cron(
					tempCron[0] + ' ' + tempCron[1] + ' ' + tempCron[2] + ' ' +
					tempCron[3] + ' ' + tempCron[4] + ' ' + tempCron[5] + ' ' + tempCron[6]
				);
			}
		}
	}

	template<size_t bits>
	void SchedulerRecord::parseItem(const std::string &item, int min, int max, std::bitset<bits> &bitset, int offset) {
		size_t pos;
		int val = 0;
		if (item == "*") {
			for (int i = min; i <= max; ++i) {
				bitset.set(i + offset, true);
			}
		} else if ((pos = item.find('/')) != std::string::npos) {
			if (++pos > item.size() - 1) {
				THROW_EXC_TRC_WAR(std::logic_error, "Unexpected format: " << item);
			}
			int divid = std::stoi(item.substr(pos));
			if (divid <= 0) {
				THROW_EXC_TRC_WAR(std::logic_error, "Invalid value: " << item);
			}
			val = min % divid;
			val = val == 0 ? min : min - val + divid;
			while (val <= max) {
				bitset.set(val + offset, true);
				val += divid;
			}
			val = min;
		} else if ((pos = item.find('-')) != std::string::npos) {
			if (++pos > item.size() - 1) {
				THROW_EXC_TRC_WAR(std::logic_error, "Invalid format: " << item);
			}
			int start = std::stoi(item.substr(0, pos - 1));
			int end = std::stoi(item.substr(pos));
			if (start < min || end > max || start >= end) {
				THROW_EXC_TRC_WAR(std::logic_error, "Invalid value: " << item)
			}
			for (int i = start; i <= end; ++i) {
				bitset.set(i, true);
			}
			val = min;
		} else if ((pos = item.find(',')) != std::string::npos) {
			pos = 0;
			std::string substr = item;
			while (true) {
				val = std::stoi(substr, &pos);
				if (val < min || val > max) {
					THROW_EXC_TRC_WAR(std::logic_error, "Invalid value: " << item);
				}
				bitset.set(val + offset, true);
				if (++pos > substr.size() - 1) {
					break;
				}
				substr = substr.substr(pos);
			}
			val = min;
		} else {
			val = std::stoi(item);
			if (val < min || val > max) {
				THROW_EXC_TRC_WAR(std::logic_error, "Invalid value: " << item);
			}
			bitset.set(val + offset, true);
		}
	}

	void SchedulerRecord::populateTimeSpec() {
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
		Pointer("/period").Set(m_timeSpec, (uint64_t)(m_period.count()));
		Pointer("/startTime").Set(m_timeSpec, SchedulerRecord::asString(m_startTime));
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
