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
  class MngModeMsg : public ApiMsg
  {
  public:
    MngModeMsg() = delete;
    MngModeMsg(const rapidjson::Document& doc)
      :ApiMsg(doc)
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

  private:
    IUdpConnectorService::Mode m_mode;
  };

  class MngRestartMsg : public ApiMsg
  {
  public:
    MngRestartMsg() = delete;
    MngRestartMsg(const rapidjson::Document& doc)
      :ApiMsg(doc)
    {
      m_timeToRestart = rapidjson::Pointer("/data/req/timeToRestart").Get(doc)->GetInt();
    }

    virtual ~MngRestartMsg()
    {
    }

    double getTimeToRestart() const
    {
      return m_timeToRestart;
    }

  private:
    double m_timeToRestart;
  };

  class SchedAddTaskMsg : public ApiMsg
  {
  public:
    SchedAddTaskMsg() = delete;
    SchedAddTaskMsg(const rapidjson::Document& doc)
      :ApiMsg(doc)
    {
      m_clientId = rapidjson::Pointer("/data/req/clientId").Get(doc)->GetString();
      
      std::ostringstream os;
      os <<
        rapidjson::Pointer("/data/req/cronTime/0").Get(doc)->GetString() << ' ' <<
        rapidjson::Pointer("/data/req/cronTime/1").Get(doc)->GetString() << ' ' <<
        rapidjson::Pointer("/data/req/cronTime/2").Get(doc)->GetString() << ' ' <<
        rapidjson::Pointer("/data/req/cronTime/3").Get(doc)->GetString() << ' ' <<
        rapidjson::Pointer("/data/req/cronTime/4").Get(doc)->GetString() << ' ' <<
        rapidjson::Pointer("/data/req/cronTime/5").Get(doc)->GetString() << ' ' <<
        rapidjson::Pointer("/data/req/cronTime/6").Get(doc)->GetString();

      m_cronTime = os.str();

      const Value *taskVal = Pointer("/data/req/task").Get(doc);
      if (taskVal && taskVal->IsObject()) {
        m_task.CopyFrom(*taskVal, m_task.GetAllocator());
      }
      else {
        TRC_WARNING("Expected object: /data/req/task")
      }
    }

    virtual ~SchedAddTaskMsg()
    {
    }

    const std::string& getClientId() const
    {
      return m_clientId;
    }

    const std::string& getCronTime() const
    {
      return m_cronTime;
    }

    const rapidjson::Document& getTask() const
    {
      return m_task;
    }

  private:
    std::string m_clientId;
    std::string m_cronTime;
    rapidjson::Document m_task;
  };

  class SchedPeriodicTaskMsg : public ApiMsg
  {
  public:
    SchedPeriodicTaskMsg() = delete;
    SchedPeriodicTaskMsg(const rapidjson::Document& doc)
      :ApiMsg(doc)
    {
      m_clientId = rapidjson::Pointer("/data/req/clientId").Get(doc)->GetString();
      m_period = rapidjson::Pointer("/data/req/timePeriod").Get(doc)->GetInt();

      const Value *tpVal = Pointer("/data/req/timePoint").Get(doc);
      if (tpVal && tpVal->IsString()) {
        m_point = parseTimestamp(tpVal->GetString());
      }
      else {
        TRC_WARNING("Expected object: /data/req/task")
      }

      const Value *taskVal = Pointer("/data/req/task").Get(doc);
      if (taskVal && taskVal->IsObject()) {
        m_task.CopyFrom(*taskVal, m_task.GetAllocator());
      }
      else {
        TRC_WARNING("Expected object: /data/req/task")
      }
    }

    virtual ~SchedPeriodicTaskMsg()
    {
    }

    const std::string& getClientId() const
    {
      return m_clientId;
    }

    int getPeriod() const
    {
      return m_period;
    }

    const std::chrono::system_clock::time_point& getPoint() const
    {
      return m_point;
    }

    const rapidjson::Document& getTask() const
    {
      return m_task;
    }

  private:
    std::string m_clientId;
    int m_period;
    std::chrono::system_clock::time_point m_point;
    rapidjson::Document m_task;
  };

  class SchedGetTaskMsg : public ApiMsg
  {
  public:
    SchedGetTaskMsg() = delete;
    SchedGetTaskMsg(const rapidjson::Document& doc)
      :ApiMsg(doc)
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

    const int getTaskId() const
    {
      return m_taskId;
    }

  private:
    std::string m_clientId;
    int m_taskId;
  };

  class SchedRemoveTaskMsg : public ApiMsg
  {
  public:
    SchedRemoveTaskMsg() = delete;
    SchedRemoveTaskMsg(const rapidjson::Document& doc)
      :ApiMsg(doc)
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

    const int getTaskId() const
    {
      return m_taskId;
    }

  private:
    std::string m_clientId;
    int m_taskId;
  };

  class SchedListMsg : public ApiMsg
  {
  public:
    SchedListMsg() = delete;
    SchedListMsg(const rapidjson::Document& doc)
      :ApiMsg(doc)
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

  private:
    std::string m_clientId;
  };

  class SchedRemoveAllMsg : public ApiMsg
  {
  public:
    SchedRemoveAllMsg() = delete;
    SchedRemoveAllMsg(const rapidjson::Document& doc)
      :ApiMsg(doc)
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
      "mngDaemon",
      "mngSched"
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

      if (m_iUdpConnectorService) { // interface is UNREQUIRED

                                    // switch mode
        m_iUdpConnectorService->setMode(msg.getMode());

        // prepare OK response
        Pointer("/data/rsp/operMode").Set(respDoc, ModeStringConvertor::enum2str(msg.getMode()));

        if (msg.getVerbose()) {
          Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
          Pointer("/data/statusStr").Set(respDoc, "ok");
        }

        Pointer("/data/status").Set(respDoc, 0);
      }
      else {
        // prepare ERR response
        Pointer("/data/rsp/operMode").Set(respDoc, ModeStringConvertor::enum2str(msg.getMode()));

        if (msg.getVerbose()) {
          Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
          Pointer("/data/statusStr").Set(respDoc, "ERROR UdpConnectorService not active");
        }

        Pointer("/data/status").Set(respDoc, -1);

      }
      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngDaemon_Restart(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      MngRestartMsg msg(reqDoc);

      Document d;
      Pointer("/task/restart").Set(d, true);

      TRC_INFORMATION(std::endl << "Exit scheduled in: " << msg.getTimeToRestart() << " milliseconds");
      std::cout << std::endl << "Exit scheduled in: " << msg.getTimeToRestart() << " milliseconds" << std::endl;

      m_iSchedulerService->scheduleTaskAt("JsonMngApi", d,
        std::chrono::system_clock::now() + std::chrono::milliseconds((unsigned)msg.getTimeToRestart()));

      // prepare OK response
      Pointer("/data/rsp/timeToRestart").Set(respDoc, msg.getTimeToRestart());

      if (msg.getVerbose()) {
        Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
        Pointer("/data/statusStr").Set(respDoc, "ok");
      }

      Pointer("/data/status").Set(respDoc, 0);

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngSched_AddTask(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedAddTaskMsg msg(reqDoc);

      int64_t taskId = m_iSchedulerService->scheduleTask(msg.getClientId(), msg.getTask(), msg.getCronTime()); //TODO point

      // prepare OK response
      Pointer("/data/rsp/clientId").Set(respDoc, msg.getClientId());
      Pointer("/data/rsp/taskId").Set(respDoc, taskId);

      if (msg.getVerbose()) {
        Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
        Pointer("/data/statusStr").Set(respDoc, "ok");
      }

      Pointer("/data/status").Set(respDoc, 0);

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngSched_PeriodicTask(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedPeriodicTaskMsg msg(reqDoc);

      long taskId = m_iSchedulerService->scheduleTaskPeriodic(
        msg.getClientId(), msg.getTask(), std::chrono::seconds(msg.getPeriod()/1000), msg.getPoint()); //TODO point

      // prepare OK response
      Pointer("/data/rsp/clientId").Set(respDoc, msg.getClientId());
      Pointer("/data/rsp/taskId").Set(respDoc, taskId);

      if (msg.getVerbose()) {
        Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
        Pointer("/data/statusStr").Set(respDoc, "ok");
      }

      Pointer("/data/status").Set(respDoc, 0);

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngSched_GetTask(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedGetTaskMsg msg(reqDoc);

      const Value *task = m_iSchedulerService->getMyTask(msg.getClientId(), msg.getTaskId());
      const Value *timeSpec = m_iSchedulerService->getMyTaskTimeSpec(msg.getClientId(), msg.getTaskId());
      if (task) {
        // prepare OK response
        Pointer("/data/rsp/clientId").Set(respDoc, msg.getClientId());
        Pointer("/data/rsp/taskId").Set(respDoc, msg.getTaskId());
        Pointer("/data/rsp/task").Set(respDoc, *task);
        Pointer("/data/rsp/timeSpec").Set(respDoc, *timeSpec);

        if (msg.getVerbose()) {
          Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
          Pointer("/data/statusStr").Set(respDoc, "ok");
        }

        Pointer("/data/status").Set(respDoc, 0);
      }
      else {
        // prepare err response
        Pointer("/data/rsp/clientId").Set(respDoc, msg.getClientId());
        Pointer("/data/rsp/taskId").Set(respDoc, msg.getTaskId());

        if (msg.getVerbose()) {
          Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
          Pointer("/data/statusStr").Set(respDoc, "err");
          Value empty;
          empty.SetObject();
          Pointer("/data/rsp/task").Set(respDoc, empty);
          Pointer("/data/errorStr").Set(respDoc, "clientId or taskId doesn't exist");
        }

        Pointer("/data/status").Set(respDoc, -1);
      }

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngSched_RemoveTask(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedRemoveTaskMsg msg(reqDoc);

      auto *task = m_iSchedulerService->getMyTask(msg.getClientId(), msg.getTaskId());
      if (task) {
        m_iSchedulerService->removeTask(msg.getClientId(), msg.getTaskId());
        // prepare OK response
        Pointer("/data/rsp/clientId").Set(respDoc, msg.getClientId());
        Pointer("/data/rsp/taskId").Set(respDoc, msg.getTaskId());

        if (msg.getVerbose()) {
          Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
          Pointer("/data/statusStr").Set(respDoc, "ok");
        }

        Pointer("/data/status").Set(respDoc, 0);
      }
      else {
        // prepare err response
        Pointer("/data/rsp/clientId").Set(respDoc, msg.getClientId());
        Pointer("/data/rsp/taskId").Set(respDoc, msg.getTaskId());

        if (msg.getVerbose()) {
          Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
          Pointer("/data/statusStr").Set(respDoc, "err");
          Pointer("/data/errorStr").Set(respDoc, "clientId or taskId doesn't exist");
        }

        Pointer("/data/status").Set(respDoc, -1);
      }

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngSched_List(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedListMsg msg(reqDoc);

      std::vector<ISchedulerService::TaskHandle> vect = m_iSchedulerService->getMyTasks(msg.getClientId());
      
      // prepare OK response
      Pointer("/data/rsp/clientId").Set(respDoc, msg.getClientId());
      Value arr;
      arr.SetArray();
      for (auto v : vect) {
        arr.PushBack(v, respDoc.GetAllocator());
      }
      Pointer("/data/rsp/tasks").Set(respDoc, arr);

      if (msg.getVerbose()) {
        Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
        Pointer("/data/statusStr").Set(respDoc, "ok");
      }

      Pointer("/data/status").Set(respDoc, 0);

      TRC_FUNCTION_LEAVE("");
    }

    void handleMsg_mngSched_RemoveAll(const rapidjson::Document& reqDoc, Document& respDoc)
    {
      TRC_FUNCTION_ENTER("");

      SchedRemoveAllMsg msg(reqDoc);

      m_iSchedulerService->removeAllMyTasks(msg.getClientId());
      // prepare OK response
      Pointer("/data/rsp/clientId").Set(respDoc, msg.getClientId());

      if (msg.getVerbose()) {
        Pointer("/data/insId").Set(respDoc, "iqrfgd2-1"); // TODO replace by daemon instance id
        Pointer("/data/statusStr").Set(respDoc, "ok");
      }

      Pointer("/data/status").Set(respDoc, 0);

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
      else if (msgType.m_type == "mngDaemon_Restart") {
        handleMsg_mngDaemon_Restart(doc, respDoc);
      }
      else if (msgType.m_type == "mngSched_AddTask") {
        handleMsg_mngSched_AddTask(doc, respDoc);
      }
      else if (msgType.m_type == "mngSched_PeriodicTask") {
        handleMsg_mngSched_PeriodicTask(doc, respDoc);
      }
      else if (msgType.m_type == "mngSched_GetTask") {
        handleMsg_mngSched_GetTask(doc, respDoc);
      }
      else if (msgType.m_type == "mngSched_RemoveTask") {
        handleMsg_mngSched_RemoveTask(doc, respDoc);
      }
      else if (msgType.m_type == "mngSched_RemoveAll") {
        handleMsg_mngSched_RemoveAll(doc, respDoc);
      }
      else if (msgType.m_type == "mngSched_List") {
        handleMsg_mngSched_List(doc, respDoc);
      }
      else {
        //TODO not support
      }

      //TODO validate response in debug
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void handleSchedulerMsg(const rapidjson::Value& val)
    {
      TRC_INFORMATION(std::endl << "Scheduled Exit ... " << std::endl);
      std::cout << std::endl << "Scheduled Exit ... " << std::endl;
      m_iLaunchService->exit();
    }

    void activate(const shape::Properties *props)
    {
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
