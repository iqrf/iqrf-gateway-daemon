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
    std::map<int, std::shared_ptr<Context>> m_contexts;

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

    //##########################################
    int loadJsCodeContext(int contextId, const std::string& js)
    {
      TRC_FUNCTION_ENTER(PAR(contextId));
      auto found = m_contexts.find(contextId);
      if (found != m_contexts.end()) {
        //THROW_EXC_TRC_WAR(std::logic_error, "Already created JS context: " << PAR(id));
        m_contexts.erase(contextId);
      }
      auto res = m_contexts.insert(std::make_pair(contextId, std::shared_ptr<Context>(shape_new Context())));
      res.first->second->loadJsCode(js);
      TRC_FUNCTION_LEAVE("");
      return 0;
    }

    void callContext(int contextId, const std::string& functionName, const std::string& par, std::string& ret)
    {
      TRC_FUNCTION_ENTER(PAR(contextId) << PAR(functionName));
      auto found = m_contexts.find(contextId);
      if (found != m_contexts.end()) {
        found->second->call(functionName, par, ret);
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot find context: " << PAR_HEX(contextId) << PAR(functionName));
      }
      TRC_FUNCTION_LEAVE("");
    }

    int loadJsCodeFenced(int id, const std::string& js)
    {
      TRC_FUNCTION_ENTER("");
      auto found = m_contexts.find(id);
      if (found != m_contexts.end()) {
        //THROW_EXC_TRC_WAR(std::logic_error, "Already created JS context: " << PAR(id));
        m_contexts.erase(id);
      }
      auto res = m_contexts.insert(std::make_pair(id, std::shared_ptr<Context>(shape_new Context())));
      res.first->second->loadJsCode(js);
      TRC_FUNCTION_LEAVE("");
      return 0;
    }

    void callFenced(int id, const std::string& functionName, const std::string& par, std::string& ret)
    {
      TRC_FUNCTION_ENTER(PAR(id) << PAR(functionName));
      auto found = m_contexts.find(id);
      if (found == m_contexts.end()) {
        call(functionName, par, ret);
      }
      else {
        found->second->call(functionName, par, ret);
      }
      TRC_FUNCTION_LEAVE("");
    }
    //##########################################

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

    void activate(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsRenderDuktape instance activate" << std::endl <<
        "******************************"
      );

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

  void JsRenderDuktape::loadJsCodeContext(int contextId, const std::string& js)
  {
    m_imp->loadJsCodeContext(contextId, js);
  }

  void JsRenderDuktape::callContext(int contextId, const std::string& functionName, const std::string& par, std::string& ret)
  {
    m_imp->callContext(contextId, functionName, par, ret);
  }

  void JsRenderDuktape::loadJsCodeFenced(int id, const std::string& js)
  {
    m_imp->loadJsCodeFenced(id, js);
  }

  void JsRenderDuktape::callFenced(int context, const std::string& functionName, const std::string& par, std::string& ret)
  {
    m_imp->callFenced(context, functionName, par, ret);
  }

  void JsRenderDuktape::loadJsCode(const std::string& js)
  {
    m_imp->loadJsCode(js);
  }

  void JsRenderDuktape::call(const std::string& functionName, const std::string& par, std::string& ret)
  {
    m_imp->call(functionName, par, ret);
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
