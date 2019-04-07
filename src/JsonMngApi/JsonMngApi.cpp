#include "VersionInfo.h"
#include "ApiMsg.h"
#include "JsonMngApi.h"
#include "HexStringCoversion.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>
#include <iostream>

#include "iqrf__JsonMngApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonMngApi);

using namespace rapidjson;

namespace iqrf {
  class MngMsg : public ApiMsg
  {
  public:
    MngMsg() = delete;
    MngMsg(const rapidjson::Document& doc)
      :ApiMsg(doc)
    {
    }

    virtual ~MngMsg()
    {
    }

    const std::string& getErr() const
    {
      return m_errStr;
    }

    void setErr(const std::string& errStr)
    {
      m_errStr = errStr;
      m_success = false;
    }

    bool isSuccess()
    {
      return m_success;
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      if (m_success) {
        setStatus("ok", 0);
      }
      else {
        if (getVerbose()) {
          Pointer("/data/errorStr").Set(doc, m_errStr);
        }
        setStatus("err", -1);
      }
    }

  private:
    std::string m_errStr;
    bool m_success = true;
  };

  class MngModeMsg : public MngMsg
  {
  public:
    MngModeMsg() = delete;
    MngModeMsg(const rapidjson::Document& doc)
      :MngMsg(doc)
    {
      m_mode = ModeStringConvertor::str2enum(rapidjson::Pointer("/data/req/operMode").Get(doc)->GetString());
    }

    virtual ~MngModeMsg()
    {
    }

    IUdpConnectorService::Mode getMode() const
    {
      return m_mode;
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      Pointer("/data/rsp/operMode").Set(doc, ModeStringConvertor::enum2str(m_mode));
      MngMsg::createResponsePayload(doc);
    }

  private:
    IUdpConnectorService::Mode m_mode;
  };

  class MngRestartMsg : public MngMsg
  {
  public:
    MngRestartMsg() = delete;
    MngRestartMsg(const rapidjson::Document& doc)
      :MngMsg(doc)
    {
      m_timeToExit = rapidjson::Pointer("/data/req/timeToExit").Get(doc)->GetDouble();
    }

    virtual ~MngRestartMsg()
    {
    }

    double getTimeToRestart() const
    {
      return m_timeToExit;
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      Pointer("/data/rsp/timeToExit").Set(doc, m_timeToExit);
      MngMsg::createResponsePayload(doc);
    }

  private:
    double m_timeToExit;
  };

  class MngVersionMsg : public MngMsg
  {
  public:
    MngVersionMsg() = delete;
    MngVersionMsg(const rapidjson::Document& doc)
      :MngMsg(doc)
      , m_buildTimestamp(BUILD_TIMESTAMP)
      , m_daemonVersion(DAEMON_VERSION)
    {
    }

    virtual ~MngVersionMsg()
    {
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      std::stringstream os;
      os << m_daemonVersion << ' ' << m_buildTimestamp;
      Pointer("/data/rsp/version").Set(doc, os.str());
      MngMsg::createResponsePayload(doc);
    }

  private:
    std::string m_buildTimestamp;
    std::string m_daemonVersion;
  };

  class SchedAddTaskMsg : public MngMsg
  {
  public:
    SchedAddTaskMsg() = delete;
    SchedAddTaskMsg(const rapidjson::Document& doc)
      :MngMsg(doc)
    {
      using namespace rapidjson;

      m_clientId = Pointer("/data/req/clientId").Get(doc)->GetString();
      
      const Value* cron = Pointer("/data/req/timeSpec/cronTime").Get(doc);
      auto it = cron->Begin();
      for (int i = 0; i < 7; i++) {
        m_cron[i] = it->GetString();
        it++;
      }

      m_periodic = Pointer("/data/req/timeSpec/periodic").Get(doc)->GetBool();
      m_period = Pointer("/data/req/timeSpec/period").Get(doc)->GetInt();
      m_exactTime = Pointer("/data/req/timeSpec/exactTime").Get(doc)->GetBool();

      const Value *tpVal = Pointer("/data/req/timeSpec/startTime").Get(doc);
      m_startTime = parseTimestamp(tpVal->GetString());

      const Value *taskVal = Pointer("/data/req/task").Get(doc);
      if (taskVal && taskVal->IsObject()) {
        m_task.CopyFrom(*taskVal, m_task.GetAllocator());
      }
      else {
        TRC_WARNING("Expected object: /data/req/task")
      }

      const Value* persistVal = Pointer("/data/req/persist").Get(doc);
      if (persistVal && persistVal->IsBool()) {
        m_persist = persistVal->GetBool();
      }
    }

    virtual ~SchedAddTaskMsg()
    {
    }

    const std::string& getClientId() const
    {
      return m_clientId;
    }

    const ISchedulerService::CronType& getCron() const
    {
      return m_cron;
    }

    bool isPeriodic() const
    {
      return m_periodic;
    }

    int getPeriod() const
    {
      return m_period;
    }

    bool isExactTime() const
    {
      return m_exactTime;
    }

    const std::chrono::system_clock::time_point& getStartTime() const
    {
      return m_startTime;
    }

    const rapidjson::Document& getTask() const
    {
      return m_task;
    }

    int64_t getTaskId() const
    {
      return m_taskId;
    }

    void setTaskId(int64_t taskId)
    {
      m_taskId = taskId;
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      Pointer("/data/rsp/clientId").Set(doc, m_clientId);
      Pointer("/data/rsp/taskId").Set(doc, m_taskId);
      MngMsg::createResponsePayload(doc);
    }

    bool getPersist() const { return m_persist; }

  private:
    std::string m_clientId;
    ISchedulerService::CronType m_cron;
    bool m_periodic = false;
    int m_period = 0;
    bool m_exactTime = false;
    std::chrono::system_clock::time_point m_startTime;
    rapidjson::Document m_task;
    int64_t m_taskId = 0;
    bool m_persist = false;
  };

  class SchedGetTaskMsg : public MngMsg
  {
  public:
    SchedGetTaskMsg() = delete;
    SchedGetTaskMsg(const rapidjson::Document& doc)
      :MngMsg(doc)
    {
      m_clientId = rapidjson::Pointer("/data/req/clientId").Get(doc)->GetString();
      m_taskId = rapidjson::Pointer("/data/req/taskId").Get(doc)->GetInt();
    }

    virtual ~SchedGetTaskMsg()
    {
    }

    const std::string& getClientId() const
    {
      return m_clientId;
    }

    int getTaskId() const
    {
      return m_taskId;
    }

    const rapidjson::Value* getTask() const
    {
      return m_task;
    }

    void setTask(const rapidjson::Value *task)
    {
      m_task = task;
    }

    const rapidjson::Value* getTimeSpec() const
    {
      return m_timeSpec;
    }

    void setTimeSpec(const rapidjson::Value* timeSpec)
    {
      m_timeSpec = timeSpec;
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      Pointer("/data/rsp/clientId").Set(doc, m_clientId);
      Pointer("/data/rsp/taskId").Set(doc, m_taskId);

      if (isSuccess()) {
        // prepare OK response
        Pointer("/data/rsp/task").Set(doc, *m_task);
        Pointer("/data/rsp/timeSpec").Set(doc, *m_timeSpec);
      }
      MngMsg::createResponsePayload(doc);
    }

  private:
    std::string m_clientId;
    int m_taskId;
    const rapidjson::Value* m_task = nullptr;
    const rapidjson::Value* m_timeSpec = nullptr;
  };

  class SchedRemoveTaskMsg : public MngMsg
  {
  public:
    SchedRemoveTaskMsg() = delete;
    SchedRemoveTaskMsg(const rapidjson::Document& doc)
      :MngMsg(doc)
    {
      m_clientId = rapidjson::Pointer("/data/req/clientId").Get(doc)->GetString();
      m_taskId = rapidjson::Pointer("/data/req/taskId").Get(doc)->GetInt();
    }

    virtual ~SchedRemoveTaskMsg()
    {
    }

    const std::string& getClientId() const
    {
      return m_clientId;
    }

    int getTaskId() const
    {
      return m_taskId;
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      Pointer("/data/rsp/clientId").Set(doc, m_clientId);
      Pointer("/data/rsp/taskId").Set(doc, m_taskId);
      MngMsg::createResponsePayload(doc);
    }

  private:
    std::string m_clientId;
    int m_taskId;
    const rapidjson::Value* m_task = nullptr;
  };

  class SchedListMsg : public MngMsg
  {
  public:
    SchedListMsg() = delete;
    SchedListMsg(const rapidjson::Document& doc)
      :MngMsg(doc)
    {
      m_clientId = rapidjson::Pointer("/data/req/clientId").Get(doc)->GetString();
    }

    virtual ~SchedListMsg()
    {
    }

    const std::string& getClientId() const
    {
      return m_clientId;
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      using namespace rapidjson;
      // prepare OK response
      Pointer("/data/rsp/clientId").Set(doc, m_clientId);
      Value arr;
      arr.SetArray();
      for (auto v : m_vect) {
        arr.PushBack(v, doc.GetAllocator());
      }
      Pointer("/data/rsp/tasks").Set(doc, arr);

      MngMsg::createResponsePayload(doc);
    }

    void setVect(const std::vector<ISchedulerService::TaskHandle>& vect)
    {
      m_vect = vect;
    }

  private:
    std::string m_clientId;
    std::vector<ISchedulerService::TaskHandle> m_vect;
  };

  class SchedRemoveAllMsg : public MngMsg
  {
  public:
    SchedRemoveAllMsg() = delete;
    SchedRemoveAllMsg(const rapidjson::Document& doc)
      :MngMsg(doc)
    {
      m_clientId = rapidjson::Pointer("/data/req/clientId").Get(doc)->GetString();
    }

    virtual ~SchedRemoveAllMsg()
    {
    }

    const std::string& getClientId() const
    {
      return m_clientId;
    }

    void createResponsePayload(rapidjson::Document& doc) override
    {
      Pointer("/data/rsp/clientId").Set(doc, m_clientId);
      MngMsg::createResponsePayload(doc);
    }

  private:
    std::string m_clientId;
  };

  class JsonMngApi::Imp
  {
  private:

    shape::ILaunchService* m_iLaunchService = nullptr;
    ISchedulerService* m_iSchedulerService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IUdpConnectorService* m_iUdpConnectorService = nullptr;

    std::vector<std::string> m_filters =
    {
      "mngScheduler",
      "mngDaemon"
    };

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void handleMsg_mngDaemon_Mode(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;
      MngModeMsg msg(reqDoc);

      try {
        if (m_iUdpConnectorService) { // interface is UNREQUIRED
          // switch mode
          m_iUdpConnectorService->setMode(msg.getMode());
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "UdpConnectorService not active");
        }
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Cannot handle MngModeMsg");
        msg.setErr(e.what());
      }
      msg.createResponse(respDoc);

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngDaemon_Exit(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      MngRestartMsg msg(reqDoc);

      Document d;
      Pointer("/task/restart").Set(d, true);

      TRC_INFORMATION(std::endl << "Exit scheduled in: " << msg.getTimeToRestart() << " milliseconds");
      std::cout << std::endl << "Exit scheduled in: " << msg.getTimeToRestart() << " milliseconds" << std::endl;

      m_iSchedulerService->scheduleTaskAt("JsonMngApi", d,
        std::chrono::system_clock::now() + std::chrono::milliseconds((unsigned)msg.getTimeToRestart()));

      msg.createResponse(respDoc);

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngDaemon_Version(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      MngVersionMsg msg(reqDoc);
      msg.createResponse(respDoc);

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngScheduler_AddTask(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedAddTaskMsg msg(reqDoc);
      
      int64_t taskId = 0;

      try {
        if (msg.isPeriodic()) {
          taskId = m_iSchedulerService->scheduleTaskPeriodic(
            msg.getClientId(), msg.getTask(), std::chrono::seconds(msg.getPeriod()), msg.getStartTime(), msg.getPersist());
        }
        else if (msg.isExactTime()) {
          taskId = m_iSchedulerService->scheduleTaskAt(
            msg.getClientId(), msg.getTask(), msg.getStartTime(), msg.getPersist());
        }
        else {
          //cron
          taskId = m_iSchedulerService->scheduleTask(
            msg.getClientId(), msg.getTask(), msg.getCron(), msg.getPersist());
        }
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Cannot schedule task " << NAME_PAR(msg.getClientId, msg.getClientId()));
        std::ostringstream os;
        os << "Cannot schedule task: " << e.what();
        msg.setErr(os.str());
      }

      msg.setTaskId(taskId);

      msg.createResponse(respDoc);

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngScheduler_GetTask(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedGetTaskMsg msg(reqDoc);

      const Value *task = m_iSchedulerService->getMyTask(msg.getClientId(), msg.getTaskId());
      const Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(msg.getClientId(), msg.getTaskId());
      
      msg.setTask(task);
      msg.setTimeSpec(timeSpec);
      
      if (!task) {
        msg.setErr("clientId or taskId doesn't exist");
      }

      msg.createResponse(respDoc);
      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngScheduler_RemoveTask(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedRemoveTaskMsg msg(reqDoc);

      const Value *task = m_iSchedulerService->getMyTask(msg.getClientId(), msg.getTaskId());
      if (task) {
        m_iSchedulerService->removeTask(msg.getClientId(), msg.getTaskId());
      }
      else {
        msg.setErr("clientId or taskId doesn't exist");
      }

      msg.createResponse(respDoc);
      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngScheduler_List(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedListMsg msg(reqDoc);

      std::vector<ISchedulerService::TaskHandle> vect = m_iSchedulerService->getMyTasks(msg.getClientId());
      msg.setVect(vect);

      msg.createResponse(respDoc);
      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngScheduler_RemoveAll(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedRemoveAllMsg msg(reqDoc);
      msg.createResponse(respDoc);

      m_iSchedulerService->removeAllMyTasks(msg.getClientId());

      msg.createResponse(respDoc);
      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      Document respDoc;
      if (msgType.m_type == "mngDaemon_Mode") {
        handleMsg_mngDaemon_Mode(doc, respDoc);
      }
      else if (msgType.m_type == "mngDaemon_Exit") {
        handleMsg_mngDaemon_Exit(doc, respDoc);
      }
      else if (msgType.m_type == "mngDaemon_Version") {
        handleMsg_mngDaemon_Version(doc, respDoc);
      }
      else if (msgType.m_type == "mngScheduler_AddTask") {
        handleMsg_mngScheduler_AddTask(doc, respDoc);
      }
      else if (msgType.m_type == "mngScheduler_GetTask") {
        handleMsg_mngScheduler_GetTask(doc, respDoc);
      }
      else if (msgType.m_type == "mngScheduler_RemoveTask") {
        handleMsg_mngScheduler_RemoveTask(doc, respDoc);
      }
      else if (msgType.m_type == "mngScheduler_RemoveAll") {
        handleMsg_mngScheduler_RemoveAll(doc, respDoc);
      }
      else if (msgType.m_type == "mngScheduler_List") {
        handleMsg_mngScheduler_List(doc, respDoc);
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "unknown type: " << msgType.m_type)
        //TODO not support
      }

      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void handleSchedulerMsg(const rapidjson::Value& val)
    {
      (void)val; //silence -Wunused-parameter
      TRC_INFORMATION(std::endl << "Scheduled Exit ... " << std::endl);
      std::cout << std::endl << "Scheduled Exit ... " << std::endl;
      m_iLaunchService->exit();
    }

    void activate(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonMngApi instance activate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->registerFilteredMsgHandler(m_filters,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
      {
        handleMsg(messagingId, msgType, std::move(doc));
      });

      m_iSchedulerService->registerTaskHandler("JsonMngApi", [&](const rapidjson::Value& val)
      {
        handleSchedulerMsg(val);
      });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonMngApi instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);
      m_iSchedulerService->unregisterTaskHandler("JsonMngApi");

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
    }

    void attachInterface(shape::ILaunchService* iface)
    {
      m_iLaunchService = iface;
    }

    void detachInterface(shape::ILaunchService* iface)
    {
      if (m_iLaunchService == iface) {
        m_iLaunchService = nullptr;
      }
    }

    void attachInterface(ISchedulerService* iface)
    {
      m_iSchedulerService = iface;
    }

    void detachInterface(ISchedulerService* iface)
    {
      if (m_iSchedulerService == iface) {
        m_iSchedulerService = nullptr;
      }
    }

    void attachInterface(IUdpConnectorService* iface)
    {
      m_iUdpConnectorService = iface;
    }

    void detachInterface(IUdpConnectorService* iface)
    {
      if (m_iUdpConnectorService == iface) {
        m_iUdpConnectorService = nullptr;
      }
    }

    void attachInterface(IMessagingSplitterService* iface)
    {
      m_iMessagingSplitterService = iface;
    }

    void detachInterface(IMessagingSplitterService* iface)
    {
      if (m_iMessagingSplitterService == iface) {
        m_iMessagingSplitterService = nullptr;
      }
    }

  };

  /////////////////////////
  JsonMngApi::JsonMngApi()
  {
    m_imp = shape_new Imp();
  }

  JsonMngApi::~JsonMngApi()
  {
    delete m_imp;
  }

  void JsonMngApi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonMngApi::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonMngApi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonMngApi::attachInterface(shape::ILaunchService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonMngApi::detachInterface(shape::ILaunchService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonMngApi::attachInterface(ISchedulerService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonMngApi::detachInterface(ISchedulerService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonMngApi::attachInterface(IUdpConnectorService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonMngApi::detachInterface(IUdpConnectorService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonMngApi::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonMngApi::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonMngApi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonMngApi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
