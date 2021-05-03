#define IJsRenderService_EXPORTS

#include "JsRenderDuktape.h"
#include "duktape.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>
#include <condition_variable>

#include "iqrf__JsRenderDuktape.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsRenderDuktape);

using namespace rapidjson;

namespace iqrf {
  ///////////////////////////
  class Context
  {
  private:
    bool m_init = false;
    duk_context *m_ctx = nullptr;
    int m_relativeStack = 0;

  public:
    Context()
    {
      // Initialize.
      m_ctx = duk_create_heap_default();
      if (!m_ctx) {
        std::cerr << "Failed to create a Duktape heap.\n";
        throw std::logic_error("Failed to create a Duktape heap.\n");
      }

      duk_push_global_object(m_ctx);
    }

    ~Context()
    {
      duk_destroy_heap(m_ctx);
    }

    void loadJsCode(const std::string& js)
    {
      TRC_FUNCTION_ENTER("");

      duk_push_string(m_ctx, js.c_str());
      if (duk_peval(m_ctx) != 0) {
        std::string errstr = duk_safe_to_string(m_ctx, -1);
        std::cerr << "Error in driver scripts: " << errstr << std::endl;
        THROW_EXC_TRC_WAR(std::logic_error, errstr);
      }
      duk_pop(m_ctx);  // ignore result

      m_init = true;
      TRC_FUNCTION_LEAVE("");
    }

    void finit()
    {
      TRC_FUNCTION_ENTER("");
      //TODO duk_pop()
      m_init = false;
      TRC_FUNCTION_LEAVE("");
    }

    bool findFunc(const std::string& functionName)
    {
      TRC_FUNCTION_ENTER(PAR(functionName));
      bool retval = false;
      if (m_init && !functionName.empty()) {

        std::string buf = functionName;
        std::replace(buf.begin(), buf.end(), '.', ' ');
        std::istringstream istr(buf);

        std::vector<std::string> items;
        while (true) {
          std::string item;
          if (!(istr >> item)) {
            if (istr.eof()) break;
          }
          items.push_back(item);
        }

        retval = true;
        m_relativeStack = 0;
        for (const auto & item : items) {
          ++m_relativeStack;
          bool res = duk_get_prop_string(m_ctx, -1, item.c_str());
          if (!res) {
            duk_pop_n(m_ctx, m_relativeStack);
            THROW_EXC_TRC_WAR(std::logic_error, "Not found:: " << PAR(item));
            retval = false;
            break;
          }
        }

      }
      else {
        duk_pop_n(m_ctx, m_relativeStack);
        THROW_EXC_TRC_WAR(std::logic_error, "JS engine is not initiated");
      }
      TRC_FUNCTION_LEAVE("");
      return retval;
    }

    void call(const std::string& functionName, const std::string& par, std::string& ret)
    {
      TRC_FUNCTION_ENTER(PAR(functionName));

      if (findFunc(functionName)) {

        duk_push_string(m_ctx, par.c_str());
        duk_json_decode(m_ctx, -1);

        int res = duk_pcall(m_ctx, 1);

        std::string err;
        if (res != 0) {
          duk_dup(m_ctx, -1);
          err = duk_safe_to_string(m_ctx, -1);
          duk_pop(m_ctx);
        }

        ret = duk_json_encode(m_ctx, -1);
        if (res != 0) {
          duk_pop_n(m_ctx, m_relativeStack);
          THROW_EXC_TRC_WAR(std::logic_error, err);
        }

      }
      else {
        duk_pop_n(m_ctx, m_relativeStack);
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot find driver function: " << functionName);
      }

      duk_pop_n(m_ctx, m_relativeStack);

      TRC_FUNCTION_LEAVE("");
    }
  };

  ///////////////////////////
  class JsRenderDuktape::Imp
  {
  private:
    bool m_init = false;
    duk_context *m_ctx = nullptr;
    int m_relativeStack = 0;

    int m_ctxCounter = 0;
    mutable std::mutex m_contextMtx;
    std::map<int, std::shared_ptr<Context>> m_contexts;
    std::map<int, int> m_mapNadrContext;
    std::map<int, std::set<int>> m_mapNadrDriversId;

  public:
    Imp()
    {
      // Initialize.
      m_ctx = duk_create_heap_default();
      if (!m_ctx) {
        std::cerr << "Failed to create a Duktape heap.\n";
        throw std::logic_error("Failed to create a Duktape heap.\n");
      }

      duk_push_global_object(m_ctx);
    }

    ~Imp()
    {
      duk_destroy_heap(m_ctx);
    }

    //for debug only
    static std::string JsonToStr(const rapidjson::Value* val)
    {
      rapidjson::Document doc;
      doc.CopyFrom(*val, doc.GetAllocator());
      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      return buffer.GetString();
    }

    int loadJsCodeFenced(int contextId, const std::string& js, const std::set<int> & driverIdSet)
    {
      TRC_FUNCTION_ENTER(PAR(contextId));

      try {
        std::unique_lock<std::mutex> lck(m_contextMtx);

        auto found = m_contexts.find(contextId);
        if (found != m_contexts.end()) {
          m_contexts.erase(contextId);
        }
        auto res = m_contexts.insert(std::make_pair(contextId, std::shared_ptr<Context>(shape_new Context())));
        res.first->second->loadJsCode(js);

        m_mapNadrDriversId[contextId] = driverIdSet;

      }
      catch (std::exception & e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "cannot load passed JS code");
        // JsCache has TRC_CHANNEL 33 => write to its trace channel
        shape::Tracer::get().writeMsg((int)shape::TraceLevel::Warning, 33, TRC_MNAME, __FILE__, __LINE__, __FUNCTION__, js);
      }

      TRC_FUNCTION_LEAVE("");
      return 0;
    }

    std::set<int> getDriverIdSet(int contextId) const
    {
      std::unique_lock<std::mutex> lck(m_contextMtx);
      auto found = m_mapNadrDriversId.find(contextId);
      if (found != m_mapNadrDriversId.end()) {
        return found->second;
      }
      else {
        return std::set<int>();
      }
    }

    void mapNadrToFenced(int nadr, int contextId)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(contextId));
      std::unique_lock<std::mutex> lck(m_contextMtx);
      m_mapNadrContext[nadr] = contextId;
      TRC_FUNCTION_LEAVE("");
    }

    void callFenced(int nadr, int hwpid, const std::string& functionName, const std::string& par, std::string& ret)
    {
      TRC_FUNCTION_ENTER(PAR(nadr) << PAR(hwpid) << PAR(functionName));

      int contextId = 0;

      std::unique_lock<std::mutex> lck(m_contextMtx);

      // get drivers context according nadr
      auto fctx = m_mapNadrContext.find(nadr);
      if (fctx != m_mapNadrContext.end()) {
        // found in nadr mapping
        contextId = fctx->second;
        auto found = m_contexts.find(contextId);
        if (found != m_contexts.end()) {
          TRC_DEBUG("found according naddr mapping: " << PAR(nadr) << PAR(contextId) << PAR(functionName));
          found->second->call(functionName, par, ret);
        }
        else {
          THROW_EXC_TRC_WAR(std::logic_error, "cannot find JS context based on naddr mapping: " << PAR(nadr) << PAR(contextId) << PAR(functionName))
        }
      }
      else {
        // get provisonal driver context according hwpid
        uint16_t uhwpid = (uint16_t)hwpid;
        contextId = HWPID_MAPPING_SPACE - (int)uhwpid;
        auto found = m_contexts.find(contextId);
        if (found != m_contexts.end()) {
          // found according exact hwpid
          TRC_DEBUG("found according provisional hwpid mapping: " << PAR(uhwpid) << PAR(contextId) << PAR(functionName));
          found->second->call(functionName, par, ret);
        }
        else {
          // found default provisional context
          contextId = HWPID_DEFAULT_MAPPING;
          found = m_contexts.find(contextId);
          if (found != m_contexts.end()) {
            // found in default provisional context
            TRC_DEBUG("found according provisional hwpid default mapping: " << PAR(uhwpid) << PAR(contextId) << PAR(functionName));
            found->second->call(functionName, par, ret);
          }
          else {
            THROW_EXC_TRC_WAR(std::logic_error, "cannot find any context: " << PAR(nadr) << PAR(uhwpid) << PAR(contextId) << PAR(functionName))
          }
        }
      }

      TRC_FUNCTION_LEAVE("");
    }

    void unloadProvisionalContexts()
    {
      TRC_FUNCTION_ENTER("");

      std::unique_lock<std::mutex> lck(m_contextMtx);

      auto it = m_contexts.begin();
      while (it != m_contexts.end()) {
        if (it->first < 0) {
          it = m_contexts.erase(it);
        }
        else {
          ++it;
        }
      }
      TRC_FUNCTION_LEAVE("");
    }

    void finit()
    {
      TRC_FUNCTION_ENTER("");
      //TODO duk_pop()
      m_init = false;
      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsRenderDuktape instance activate" << std::endl <<
        "******************************"
      );
      auto thr = pthread_self();
      pthread_setname_np(thr, "igdRenderDuktape");

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsRenderDuktape instance deactivate" << std::endl <<
        "******************************"
      );

      finit();

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
    }

  };

  /////////////////////////
  JsRenderDuktape::JsRenderDuktape()
  {
    m_imp = shape_new Imp();
  }

  JsRenderDuktape::~JsRenderDuktape()
  {
    delete m_imp;
  }

  void JsRenderDuktape::loadJsCodeFenced(int contextId, const std::string& js, const std::set<int> & driverIdSet)
  {
    m_imp->loadJsCodeFenced(contextId, js, driverIdSet);
  }

  std::set<int> JsRenderDuktape::getDriverIdSet(int contextId) const
  {
    return m_imp->getDriverIdSet(contextId);
  }

  void JsRenderDuktape::mapNadrToFenced(int nadr, int contextId)
  {
    m_imp->mapNadrToFenced(nadr, contextId);
  }

  void JsRenderDuktape::callFenced(int nadr, int hwpid, const std::string& functionName, const std::string& par, std::string& ret)
  {
    m_imp->callFenced(nadr, hwpid, functionName, par, ret);
  }

  void JsRenderDuktape::unloadProvisionalContexts()
  {
    m_imp->unloadProvisionalContexts();
  }
  
  void JsRenderDuktape::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsRenderDuktape::deactivate()
  {
    m_imp->deactivate();
  }

  void JsRenderDuktape::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsRenderDuktape::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsRenderDuktape::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
