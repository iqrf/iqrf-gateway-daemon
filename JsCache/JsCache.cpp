#define IJsCacheService_EXPORTS
#define ISchedulerService_EXPORTS

#include "JsCache.h"
#include "Trace.h"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "iqrf__JsCache.hxx"

TRC_INIT_MODULE(iqrf::JsCache);

namespace iqrf {

  class JsCache::Imp
  {
  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    const std::string& getDriver(int id) const
    {
      return "";
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsCache instance activate" << std::endl <<
        "******************************"
      );

      modify(props);

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");

      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsCache instance deactivate" << std::endl <<
        "******************************"
      );
      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
    }

    void attachInterface(iqrf::ISchedulerService* iface)
    {
      m_iSchedulerService = iface;
    }

    void detachInterface(iqrf::ISchedulerService* iface)
    {
      if (m_iSchedulerService == iface) {
        m_iSchedulerService = nullptr;
      }
    }

    void attachInterface(shape::IRestApiService* iface)
    {
      m_iRestApiService = iface;
    }

    void detachInterface(shape::IRestApiService* iface)
    {
      if (m_iRestApiService == iface) {
        m_iRestApiService = nullptr;
      }
    }


  private:
    iqrf::ISchedulerService* m_iSchedulerService = nullptr;
    shape::IRestApiService* m_iRestApiService = nullptr;
  };

  //////////////////////////////////////////////////
  JsCache::JsCache()
  {
    m_imp = shape_new Imp();
  }

  JsCache::~JsCache()
  {
    delete m_imp;
  }

  const std::string& JsCache::getDriver(int id) const
  {
    return m_imp->getDriver(id);
  }

  void JsCache::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsCache::deactivate()
  {
    m_imp->deactivate();
  }

  void JsCache::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsCache::attachInterface(iqrf::ISchedulerService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsCache::detachInterface(iqrf::ISchedulerService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsCache::attachInterface(shape::IRestApiService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsCache::detachInterface(shape::IRestApiService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsCache::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsCache::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }
}
