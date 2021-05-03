#include "MonitorService.h"

#include "Trace.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"

#include <thread>
#include <condition_variable>

#include "iqrf__MonitorService.hxx"

TRC_INIT_MODULE(iqrf::MonitorService);

namespace iqrf {

  // implementation class
  class MonitorService::Imp
  {
  private:
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IUdpConnectorService* m_iUdpConnectorService = nullptr;
    shape::IWebsocketService* m_iWebsocketService = nullptr;
    std::thread m_runThread;
    bool m_runThreadFlag = true;
    std::mutex m_mtx;
    std::condition_variable m_cond;
    int m_reportPeriod = 20;

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    int getDpaQueueLen() const
    {
      return m_iIqrfDpaService->getDpaQueueLen();
    }

    IIqrfChannelService::State getIqrfChannelState()
    {
      return m_iIqrfDpaService->getIqrfChannelState();
    }

    IIqrfDpaService::DpaState getDpaChannelState()
    {
      return m_iIqrfDpaService->getDpaChannelState();
    }

    void worker() {
      TRC_FUNCTION_ENTER("");

      static unsigned num = 0;
      int dpaQueueLen = -1;
      int msgQueueLen = -1;
      IIqrfChannelService::State iqrfChannelState = IIqrfChannelService::State::NotReady;
      IIqrfDpaService::DpaState dpaChannelState = IIqrfDpaService::DpaState::NotReady;
      IUdpConnectorService::Mode operMode = IUdpConnectorService::Mode::Unknown;

      while (m_runThreadFlag) {

        std::unique_lock<std::mutex> lck(m_mtx);
        m_cond.wait_for(lck, std::chrono::seconds(m_reportPeriod));

        using namespace rapidjson;
        
        if (m_iIqrfDpaService) {
          dpaQueueLen = m_iIqrfDpaService->getDpaQueueLen();
          iqrfChannelState = m_iIqrfDpaService->getIqrfChannelState();
          dpaChannelState = m_iIqrfDpaService->getDpaChannelState();
        }

        if (m_iMessagingSplitterService) {
          msgQueueLen = m_iMessagingSplitterService->getMsgQueueLen();
        }

        if (m_iUdpConnectorService) {
          operMode = m_iUdpConnectorService->getMode();
        }

        auto ts = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        Document doc;
        Pointer("/mType").Set(doc, "ntfDaemon_Monitor");
        Pointer("/data/num").Set(doc, num++);
        Pointer("/data/timestamp").Set(doc, ts);
        Pointer("/data/dpaQueueLen").Set(doc, dpaQueueLen);
        Pointer("/data/iqrfChannelState").Set(doc, IIqrfChannelService::StateStringConvertor::enum2str(iqrfChannelState));
        Pointer("/data/dpaChannelState").Set(doc, IIqrfDpaService::DpaStateStringConvertor::enum2str(dpaChannelState));
        Pointer("/data/msgQueueLen").Set(doc, msgQueueLen);
        Pointer("/data/operMode").Set(doc, ModeStringConvertor::enum2str(operMode));

        std::string gwMonitorRecord;
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        gwMonitorRecord = buffer.GetString();

        m_iWebsocketService->sendMessage(gwMonitorRecord, ""); //send to all connected clients
      }

      TRC_FUNCTION_LEAVE("");
    }

  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************************" << std::endl <<
        "MonitorService instance activate" << std::endl <<
        "******************************************"
      );
      auto thr = pthread_self();
      pthread_setname_np(thr, "igdMonitor");

      modify(props);

      m_runThreadFlag = true;
      m_runThread = std::thread([&]() {
        worker();
      });

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "**************************************" << std::endl <<
        "MonitorService instance deactivate" << std::endl <<
        "**************************************"
      );

      m_runThreadFlag = false;
      m_cond.notify_all();
      if (m_runThread.joinable())
        m_runThread.join();

      TRC_FUNCTION_LEAVE("");
    }

    void modify(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      using namespace rapidjson;

      const Document& doc = props->getAsJson();

      {
        const Value* v = Pointer("/reportPeriod").Get(doc);
        if (v && v->IsInt()) {
          m_reportPeriod = v->GetInt();
        }
      }
      TRC_FUNCTION_LEAVE("");
    }

    void attachInterface(IIqrfDpaService* iface)
    {
      m_iIqrfDpaService = iface;
    }

    void detachInterface(IIqrfDpaService* iface)
    {
      if (m_iIqrfDpaService == iface) {
        m_iIqrfDpaService = nullptr;
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

    void attachInterface(shape::IWebsocketService* iface)
    {
      m_iWebsocketService = iface;
    }

    void detachInterface(shape::IWebsocketService* iface)
    {
      if (m_iWebsocketService == iface) {
        m_iWebsocketService = nullptr;
      }
    }

  };

  MonitorService::MonitorService()
  {
    m_imp = shape_new Imp();
  }

  MonitorService::~MonitorService()
  {
    delete m_imp;
  }

  int MonitorService::getDpaQueueLen() const
  {
    return m_imp->getDpaQueueLen();
  }

  IIqrfChannelService::State MonitorService::getIqrfChannelState()
  {
    return m_imp->getIqrfChannelState();
  }

  IIqrfDpaService::DpaState MonitorService::getDpaChannelState()
  {
    return m_imp->getDpaChannelState();
  }

  void MonitorService::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void MonitorService::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void MonitorService::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void MonitorService::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void MonitorService::attachInterface(IUdpConnectorService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void MonitorService::detachInterface(IUdpConnectorService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void MonitorService::attachInterface(shape::IWebsocketService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void MonitorService::detachInterface(shape::IWebsocketService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void MonitorService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void MonitorService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void MonitorService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void MonitorService::deactivate()
  {
    m_imp->deactivate();
  }

  void MonitorService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}
