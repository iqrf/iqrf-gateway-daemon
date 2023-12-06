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
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "iqrf__Scheduler.hxx"

TRC_INIT_MODULE(iqrf::Scheduler)

using namespace std::chrono;

namespace iqrf {

	Scheduler::Scheduler() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("")
	}

	Scheduler::~Scheduler() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// component lifecycle /////

	void Scheduler::activate(const shape::Properties *props) {
		using namespace rapidjson;
		(void)props; // silence -Wunused-parameter

		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "Scheduler instance activate" << std::endl
			<< "******************************"
		);

		std::string cacheDir = m_iLaunchService->getCacheDir();
		m_cacheDir = cacheDir.empty() ? "." : cacheDir;
		m_cacheDir += "/scheduler";

		std::string schemaDir = m_iLaunchService->getDataDir();
		m_schemaFile = schemaDir.empty() ? "." : schemaDir;
		m_schemaFile += "/schedulerSchemas/schema_cache_record.json";

		TRC_INFORMATION("Using cache dir: " << PAR(m_cacheDir));
		TRC_INFORMATION("Using record schema file: " << PAR(m_schemaFile));

		Document sd;
		std::ifstream ifs(m_schemaFile);
		if (!ifs.is_open()) {
			THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(m_schemaFile));
		}

		IStreamWrapper isw(ifs);
		sd.ParseStream(isw);

		if (sd.HasParseError()) {
			THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " << NAME_PAR(emsg, sd.GetParseError()) << NAME_PAR(eoffset, sd.GetErrorOffset()));
		}

		m_schema = std::shared_ptr<SchemaDocument>(new SchemaDocument(sd));

		loadCache();

		m_dpaTaskQueue = new TaskQueue<SchedulerRecord>([&](const SchedulerRecord &record) {
			handleScheduledRecord(record);
		});

		m_scheduledTaskPushed = false;
		m_runTimerThread = true;
		m_timerThread = std::thread(&Scheduler::worker, this);

		TRC_INFORMATION("Scheduler started");

		TRC_FUNCTION_LEAVE("")
	}

	void Scheduler::modify(const shape::Properties *props) {
		using namespace rapidjson;
		const Document &propDoc = props->getAsJson();
		{
			StringBuffer buffer;
			PrettyWriter<StringBuffer> writer(buffer);
			propDoc.Accept(writer);
			std::string cfgStr = buffer.GetString();
			TRC_DEBUG(std::endl << cfgStr);
		}
	}

	void Scheduler::deactivate() {
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

		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "Scheduler instance deactivate" << std::endl
			<< "******************************"
		);
		TRC_FUNCTION_LEAVE("")
	}

	///// public methods

	void Scheduler::registerTaskHandler(const std::string &clientId, TaskHandlerFunc fun) {
		std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
		m_messageHandlers.insert(make_pair(clientId, fun));
	}

	void Scheduler::unregisterTaskHandler(const std::string &clientId) {
		std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
		m_messageHandlers.erase(clientId);
	}

	std::vector<ISchedulerService::TaskHandle> Scheduler::getTaskIds(const std::string &clientId) const {
		std::vector<TaskHandle> tasks;
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		for (auto &task : m_tasksMap) {
			if (task.second->getClientId() != clientId) {
				continue;
			}
			tasks.push_back(task.second->getTaskId());
		}
		return tasks;
	}

	std::vector<rapidjson::Value *> Scheduler::getTasks(const std::string &clientId, rapidjson::Document::AllocatorType &allocator) const {
		std::vector<rapidjson::Value *> tasks;
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		for (auto &task : m_tasksMap) {
			if (task.second->getClientId() != clientId) {
				continue;
			}
			auto val = new rapidjson::Value(task.second->serialize(allocator));
			rapidjson::Pointer("/active").Set(*val, task.second->isActive(), allocator);
			tasks.push_back(val);
		}
		return tasks;
	}

	void Scheduler::getTaskDocument(const std::string &clientId, const TaskHandle &taskId, rapidjson::Document &doc) const {
		using namespace rapidjson;
		std::shared_ptr<SchedulerRecord> record;
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		auto item = m_tasksMap.find(taskId);
		if (item == m_tasksMap.end() || clientId != item->second->getClientId()) {
			throw std::logic_error("Client or task ID does not exist.");
		}
		record = item->second;
		Document::AllocatorType &a = doc.GetAllocator();
		Pointer("/clientId").Set(doc, record->getClientId(), a);
		Pointer("/taskId").Set(doc, record->getTaskId(), a);
		Pointer("/description").Set(doc, record->getDescription(), a);
		Pointer("/task").Set(doc, record->getTask(), a);
		Pointer("/timeSpec").Set(doc, record->getTimeSpec(), a);
		Pointer("/persist").Set(doc, record->isPersistent(), a);
		Pointer("/enabled").Set(doc, record->isStartupTask(), a);
	}

	const rapidjson::Value* Scheduler::getTask(const std::string &clientId, const TaskHandle &taskId) const {
		const rapidjson::Value *task = nullptr;
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		auto record = m_tasksMap.find(taskId);
		if (record != m_tasksMap.end() && clientId == record->second->getClientId()) {
			task = &record->second->getTask();
		}
		return task;
	}

	const rapidjson::Value* Scheduler::getTaskTimeSpec(const std::string &clientId, const TaskHandle &taskId) const {
		const rapidjson::Value *timeSpec = nullptr;
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		auto record = m_tasksMap.find(taskId);
		if (record != m_tasksMap.end() && clientId == record->second->getClientId()) {
			timeSpec = &record->second->getTimeSpec();
		}
		return timeSpec;
	}

	bool Scheduler::isTaskPersistent(const std::string &clientId, const TaskHandle &taskId) const {
		bool persistent = false;
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		auto record = m_tasksMap.find(taskId);
		if (record != m_tasksMap.end() && clientId == record->second->getClientId()) {
			persistent = record->second->isPersistent();
		}
		return persistent;
	}

	bool Scheduler::isStartupTask(const std::string &clientId, const TaskHandle &taskId) const {
		bool enabled = false;
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		auto record = m_tasksMap.find(taskId);
		if (record != m_tasksMap.end() && clientId == record->second->getClientId()) {
			enabled = record->second->isStartupTask();
		}
		return enabled;
	}

	bool Scheduler::isTaskActive(const std::string &clientId, const TaskHandle &taskId) const {
		bool active = false;
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		auto record = m_tasksMap.find(taskId);
		if (record != m_tasksMap.end() && clientId == record->second->getClientId()) {
			active = record->second->isActive();
		}
		return active;
	}

	ISchedulerService::TaskHandle Scheduler::scheduleInternalTask(
		const std::string &clientId,
		const std::string &taskId,
		const rapidjson::Value &task,
		const std::chrono::system_clock::time_point& tp,
		bool persist,
		bool enabled
	) {
		auto record = std::shared_ptr<SchedulerRecord>(
			new SchedulerRecord(clientId, taskId, task, tp, persist, enabled)
		);
		std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
		addSchedulerTask(record, enabled);
		notifyWorker();
		return record->getTaskId();
	}

	ISchedulerService::TaskHandle Scheduler::addTask(
		const std::string &clientId,
		const std::string &taskId,
		const std::string &description,
		const rapidjson::Value &task,
		const rapidjson::Value &timeSpec,
		bool persist,
		bool enabled
	) {
		using namespace rapidjson;

		std::shared_ptr<SchedulerRecord> record;

		bool periodic = Pointer("/periodic").Get(timeSpec)->GetBool();
		bool exactTime = Pointer("/exactTime").Get(timeSpec)->GetBool();
		if (periodic) { // periodic task
			uint32_t period = Pointer("/period").Get(timeSpec)->GetUint();
			record = std::shared_ptr<SchedulerRecord>(
				new SchedulerRecord(clientId, getTaskHandle(taskId), task, std::chrono::seconds(period),persist, enabled)
			);
		} else if (exactTime) { // oneshot
			std::string startTime = Pointer("/startTime").Get(timeSpec)->GetString();
			record = std::shared_ptr<SchedulerRecord>(
				new SchedulerRecord(clientId, getTaskHandle(taskId), task, false, startTime, persist, enabled)
			);
		} else { // cron
			std::string cronString = Pointer("/cronTime").Get(timeSpec)->GetString();
			record = std::shared_ptr<SchedulerRecord>(
				new SchedulerRecord(clientId, getTaskHandle(taskId), task, true, cronString, persist, enabled)
			);
		}
		record->setDescription(description);
		std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
		addSchedulerTask(record, enabled);
		notifyWorker();
		return record->getTaskId();
	}

	ISchedulerService::TaskHandle Scheduler::editTask(
		const std::string &clientId,
		const std::string &taskId,
		const std::string &newTaskId,
		const std::string &description,
		const rapidjson::Value &task,
		const rapidjson::Value &timeSpec,
		bool persist,
		bool enabled
	) {
		using namespace rapidjson;
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		auto item = m_tasksMap.find(taskId);
		if (item == m_tasksMap.end() || clientId != item->second->getClientId()) {
			throw std::logic_error("Client or task ID does not exist.");
		}
		SchedulerRecord record = *item->second;
		bool reload = false;
		if (taskId != newTaskId) {
			record.setTaskId(newTaskId);
			reload = true;
		}
		record.setTask(task);
		if (timeSpec != record.getTimeSpec()) {
			record.setTimeSpec(timeSpec);
			reload = true;
		}
		if (description != record.getDescription()) {
			record.setDescription(description);
		}
		record.setPersistence(persist);
		record.setStartupTask(enabled);
		std::shared_ptr<SchedulerRecord> ptr = std::make_shared<SchedulerRecord>(record);
		if (reload) {
			removeSchedulerTask(item->second);
			addSchedulerTask(ptr, ptr->isActive());
			notifyWorker();
		} else {
			if (persist) {
				writeTaskFile(ptr);
			} else {
				if (item->second->isPersistent()) {
					deleteTaskFile(taskId);
				}
			}
			item->second = ptr;
		}
		return ptr->getTaskId();
	}

	void Scheduler::changeTaskState(const std::string &clientId, const TaskHandle &taskId, bool active) {
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		auto record = m_tasksMap.find(taskId);
		if (record == m_tasksMap.end() || clientId != record->second->getClientId()) {
			throw std::logic_error("Client or task ID does not exist.");
		}
		if (record->second->isActive() == active) {
			return;
		}
		if (active) {
			scheduleTask(record->second);
		} else {
			unscheduleTask(taskId);
		}
		record->second->setActive(active);
		notifyWorker();
	}

	void Scheduler::removeAllTasks(const std::string &clientId) {
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		for (auto it = m_tasksMap.begin(); it != m_tasksMap.end();) {
			if (it->second->getClientId() != clientId) {
				it++;
			}
			const TaskHandle taskId = it->second->getTaskId();
			unscheduleTask(taskId);
			if (it->second->isPersistent()) {
				deleteTaskFile(taskId);
			}
			it = m_tasksMap.erase(it);
		}
		notifyWorker();
	}

	void Scheduler::removeTask(const std::string &clientId, const TaskHandle &taskId) {
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		auto record = m_tasksMap.find(taskId);
		if (record != m_tasksMap.end() && clientId == record->second->getClientId()) {
			removeSchedulerTask(record->second);
		}
		notifyWorker();
	}

	void Scheduler::removeTasks(const std::string &clientId, std::vector<TaskHandle> &taskIds) {
		std::lock_guard<std::mutex> lock(m_scheduledTasksMutex);
		for (auto &taskId : taskIds) {
			auto record = m_tasksMap.find(taskId);
			if (record != m_tasksMap.end() && clientId == record->second->getClientId()) {
				removeSchedulerTask(record->second);
			}
		}
		notifyWorker();
	}

	///// private methods

	Scheduler::TaskHandle Scheduler::addSchedulerTask(std::shared_ptr<SchedulerRecord> &record, bool start) {
		using namespace rapidjson;
		if (record->isPersistent()) {
			writeTaskFile(record);
		}
		m_tasksMap.insert(std::make_pair(record->getTaskId(), record));
		if (start) {
			scheduleTask(record);
			record->setActive(true);
		}
		return record->getTaskId();
	}

	void Scheduler::removeSchedulerTask(std::shared_ptr<SchedulerRecord> &record) {
		TaskHandle taskId = record->getTaskId();
		unscheduleTask(taskId);
		if (record->isPersistent()) {
			deleteTaskFile(taskId);
		}
		m_tasksMap.erase(taskId);
	}

	void Scheduler::writeTaskFile(std::shared_ptr<SchedulerRecord> &record) {
		using namespace rapidjson;
		std::ostringstream os;
		os << m_cacheDir << '/' << record->getTaskId() << ".json";
		std::string fname = os.str();
		std::ifstream ifs(fname);

		Document d;
		auto v = record->serialize(d.GetAllocator());
		d.Swap(v);
		std::ofstream ofs(fname);
		OStreamWrapper osw(ofs);
		PrettyWriter<OStreamWrapper> writer(osw);
		d.Accept(writer);
		ofs.close();
#ifndef SHAPE_PLATFORM_WINDOWS
		int fd = open(fname.c_str(), O_RDWR);
		if (fd < 0) {
			TRC_WARNING("Failed to open file " << fname << ". " << errno << ": " << strerror(errno));
		} else {
			if (fsync(fd) < 0) {
				TRC_WARNING("Failed to sync file to filesystem." << errno << ": " << strerror(errno));
			}
			close(fd);
		}
#endif
	}

	void Scheduler::deleteTaskFile(const TaskHandle &taskId) {
		std::ostringstream os;
		os << m_cacheDir << '/' << taskId << ".json";
		std::string fname = os.str();
		std::remove(fname.c_str());
	}

	void Scheduler::scheduleTask(std::shared_ptr<SchedulerRecord> &record) {
		system_clock::time_point timePoint;
		std::tm time;
		SchedulerRecord::getTime(timePoint, time);
		TRC_DEBUG(SchedulerRecord::asString(timePoint));

		system_clock::time_point next = record->getNext(timePoint, time);
		m_scheduledTasksMap.insert(std::make_pair(next, record->getTaskId()));
	}

	void Scheduler::unscheduleTask(const TaskHandle &taskId) {
		for (auto it = m_scheduledTasksMap.begin(); it != m_scheduledTasksMap.end();) {
			if (it->second == taskId) {
				it = m_scheduledTasksMap.erase(it);
			} else {
				it++;
			}
		}
	}

	void Scheduler::worker() {
		system_clock::time_point timePoint;
		std::tm time;
		SchedulerRecord::getTime(timePoint, time);
		TRC_DEBUG(SchedulerRecord::asString(timePoint));

		while (m_runTimerThread) {
			{
				std::unique_lock<std::mutex> lock(m_conditionVariableMutex);
				m_conditionVariable.wait_until(lock, timePoint, [&] {
					return m_scheduledTaskPushed;
				});
				m_scheduledTaskPushed = false;
			}

			// get current time
			SchedulerRecord::getTime(timePoint, time);

			while (m_runTimerThread) {
				m_scheduledTasksMutex.lock();

				if (m_scheduledTasksMap.empty()) {
					getNextWorkerCycleTime(timePoint);
					break;
				}

				auto firstTask = m_scheduledTasksMap.begin();
				std::shared_ptr<SchedulerRecord> record = m_tasksMap[firstTask->second];

				if (firstTask->first < timePoint) {
					// remove executed task
					m_scheduledTasksMap.erase(firstTask);
					// calculate next execution time
					system_clock::time_point next = record->getNext(timePoint, time);
					if (next >= timePoint) {
						m_scheduledTasksMap.insert(std::make_pair(next, record->getTaskId()));
					} else {
						removeSchedulerTask(record);
					}
					getNextWorkerCycleTime(timePoint);
					m_dpaTaskQueue->pushToQueue(*record);
				} else {
					getNextWorkerCycleTime(timePoint);
					break;
				}
			}
		}
	}

	void Scheduler::getNextWorkerCycleTime(std::chrono::system_clock::time_point &timePoint) {
		if (!m_scheduledTasksMap.empty()) {
			timePoint = m_scheduledTasksMap.begin()->first;
		} else {
			timePoint += std::chrono::seconds(10);
		}
		m_scheduledTasksMutex.unlock();
	}

	void Scheduler::notifyWorker() {
		std::unique_lock<std::mutex> lock(m_conditionVariableMutex);
		m_scheduledTaskPushed = true;
		m_conditionVariable.notify_one();
	}

	int Scheduler::handleScheduledRecord(const SchedulerRecord &record) {
		{
			std::lock_guard<std::mutex> lck(m_messageHandlersMutex);
			try {
				auto found = m_messageHandlers.find(record.getClientId());
				if (found != m_messageHandlers.end()) {
					found->second(record.getTask());
				} else {
					TRC_DEBUG("Unregistered client: " << PAR(record.getClientId()));
				}
			} catch (std::exception &e) {
				CATCH_EXC_TRC_WAR(std::exception, e, "untreated handler exception");
			}
		}
		return 0;
	}

	void Scheduler::loadCache() {
		TRC_FUNCTION_ENTER("");
		using namespace rapidjson;

		try {
			auto tfiles = getTaskFiles(m_cacheDir);

			std::set<std::string>::iterator itr = tfiles.begin();
			for (; itr != tfiles.end(); ++itr) {
				std::ifstream ifs(*itr);
				IStreamWrapper isw(ifs);

				// read document
				Document taskDoc;
				taskDoc.ParseStream(isw);
				if (taskDoc.HasParseError()) {
					TRC_WARNING("Json parse error: " << NAME_PAR(emsg, taskDoc.GetParseError()) << NAME_PAR(eoffset, taskDoc.GetErrorOffset()));
					continue; // ignore task
				}

				SchemaValidator validator(*m_schema);
				if (!taskDoc.Accept(validator)) {
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
					continue; // ignore task
				}

				bool updated = false;
				std::string filename = *itr;

				// migrate old task files
				const Value *val = Pointer("/taskId").Get(taskDoc);
				if (!val || val->IsInt()) {
					std::string uuid = generateTaskId();
					Pointer("/taskId").Set(taskDoc, uuid);
					filename = uuid;
					updated = true;
				}
				val = Pointer("/task").Get(taskDoc);
				if (val && !val->IsArray()) {
					rapidjson::Value obj(kObjectType);
					obj.CopyFrom(*val, taskDoc.GetAllocator());
					rapidjson::Value arr(kArrayType);
					arr.PushBack(obj, taskDoc.GetAllocator());
					Pointer("/task").Set(taskDoc, arr);
					updated = true;
				}
				if (!std::regex_match(basename(filename.c_str()), std::regex(TASK_FILE_PATTERN, std::regex_constants::icase))) {
					filename = val->GetString();
					updated = true;
				}

				if (updated) {
					// remove old file and create new
					std::remove((*itr).c_str());
					std::string path = m_cacheDir + "/" + filename + ".json";
					std::ofstream os(path);
					OStreamWrapper ow(os);
					PrettyWriter<OStreamWrapper> writer(ow);
					taskDoc.Accept(writer);
					os.close();
#ifndef SHAPE_PLATFORM_WINDOWS
					int fd = open(path.c_str(), O_RDWR);
					if (fd < 0) {
						TRC_WARNING("Failed to open file " << path << ". " << errno << ": " << strerror(errno));
					} else {
						if (fsync(fd) < 0) {
							TRC_WARNING("Failed to sync file to filesystem." << errno << ": " << strerror(errno));
						}
						close(fd);
					}
#endif
				}

				std::shared_ptr<SchedulerRecord> record(new SchedulerRecord(taskDoc));
				record->setPersistence(true);

				// lock and copy
				std::lock_guard<std::mutex> lck(m_scheduledTasksMutex);
				auto found = m_tasksMap.find(record->getTaskId());
				if (found != m_tasksMap.end()) {
					TRC_WARNING("Cannot load duplicit: " << NAME_PAR(taskId, record->getTaskId()))
				} else {
					m_tasksMap.insert(std::make_pair(record->getTaskId(), record));
					if (record->isStartupTask()) {
						scheduleTask(record);
						record->setActive(true);
					}
				}
			}
		} catch (std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "cannot load scheduler cache")
		}

		TRC_FUNCTION_LEAVE("");
	}

#ifdef SHAPE_PLATFORM_WINDOWS
	std::set<std::string> Scheduler::getTaskFiles(const std::string &dir) const {
		WIN32_FIND_DATA fid;
		HANDLE found = INVALID_HANDLE_VALUE;

		std::set<std::string> fileSet;
		std::string sdirect(dir);
		sdirect.append("/*.json");

		found = FindFirstFile(sdirect.c_str(), &fid);

		if (INVALID_HANDLE_VALUE == found) {
			TRC_INFORMATION("Directory does not exist or empty Scheduler cache: " << PAR(sdirect));
		}

		do {
			if (fid.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				continue; // skip a directory
			}
			std::string fil(dir);
			fil.append("/");
			fil.append(fid.cFileName);
			fileSet.insert(fil);
		} while (FindNextFile(found, &fid) != 0);

		FindClose(found);
		return fileSet;
	}
#else
	std::set<std::string> Scheduler::getTaskFiles(const std::string &dirStr) const {
		std::set<std::string> fileSet;
		std::string jsonExt = "json";

		DIR *dir;
		class dirent *ent;
		class stat st;

		dir = opendir(dirStr.c_str());
		if (dir == nullptr) {
			TRC_INFORMATION("Directory does not exist or empty Scheduler cache: " << PAR(dirStr));
		} else {
			while ((ent = readdir(dir)) != NULL) {
				const std::string file_name = ent->d_name;
				const std::string full_file_name(dirStr + "/" + file_name);

				if (file_name[0] == '.') {
					continue;
				}
				if (stat(full_file_name.c_str(), &st) == -1) {
					continue;
				}
				const bool is_directory = (st.st_mode & S_IFDIR) != 0;
				if (is_directory) {
					continue;
				}

				size_t i = full_file_name.rfind('.', full_file_name.length());
				if (i != std::string::npos && jsonExt == full_file_name.substr(i + 1, full_file_name.length() - i)) {
					fileSet.insert(full_file_name);
				} else {
					continue;
				}
			}
			closedir(dir);
		}
		return fileSet;
	}
#endif
	///// auxiliary methods /////

	std::string Scheduler::getTaskHandle(const std::string &taskId) {
		if (taskId == "00000000-0000-0000-0000-000000000000") {
			return generateTaskId();
		}
		return taskId;
	}

	std::string Scheduler::generateTaskId() {
		std::string uuid;
		do {
			uuid = boost::uuids::to_string(m_uuidGenerator());
		} while (getTask("SchedulerMessaging", uuid));
		return uuid;
	}

	///// interface management /////

	void Scheduler::attachInterface(shape::ILaunchService *iface) {
		m_iLaunchService = iface;
	}

	void Scheduler::detachInterface(shape::ILaunchService *iface) {
		if (m_iLaunchService == iface) {
			m_iLaunchService = nullptr;
		}
	}

	void Scheduler::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void Scheduler::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
