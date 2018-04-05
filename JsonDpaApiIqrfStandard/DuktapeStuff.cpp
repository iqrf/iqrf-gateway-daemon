
#include "DuktapeStuff.h"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "Trace.h"
#include <algorithm>
#include <iostream>

namespace iqrf {
  DuktapeStuff::DuktapeStuff()
  {
    // Initialize.
    m_ctx = duk_create_heap_default();
    if (!m_ctx) {
      std::cerr << "Failed to create a Duktape heap.\n";
      throw std::logic_error("Failed to create a Duktape heap.\n");
    }

    duk_push_global_object(m_ctx);
  }

  DuktapeStuff::~DuktapeStuff()
  {
    //if (m_init)
    //  finit();
    duk_destroy_heap(m_ctx);
  }

  void DuktapeStuff::init(const std::map<int, const IJsCacheService::StdDriver*>& scripts)
  {
    TRC_FUNCTION_ENTER("");
    m_scripts = scripts;
    std::string str2load;

    // agregate scripts
    for (const auto sc : m_scripts) {
      // Create a string containing the JavaScript source code.
      str2load += sc.second->getDriver();
    }

    TRC_DEBUG("loading agregated scripts: " << NAME_PAR(size, str2load.size()));
    duk_push_string(m_ctx, str2load.c_str());
    if (duk_peval(m_ctx) != 0) {
      std::cerr << "Error in driver scripts: " << duk_safe_to_string(m_ctx, -1) << std::endl;
      throw std::logic_error("");
    }
    duk_pop(m_ctx);  // ignore result

    m_init = true;
    TRC_FUNCTION_LEAVE("");
  }

  void DuktapeStuff::finit()
  {
    TRC_FUNCTION_ENTER("");
    //TODO duk_pop()
    m_init = false;
    TRC_FUNCTION_LEAVE("");
  }

  bool DuktapeStuff::findFunc(const std::string& functionName)
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
          THROW_EXC_TRC_WAR(std::logic_error, "Not found:: " << PAR(item));
          //TRC_WARNING("Not found:: " << PAR(item));
          retval = false;
          break;
        }
      }

    }
    else {
      THROW_EXC_TRC_WAR(std::logic_error, "Duktape is not initiated");
      //TRC_WARNING("Duktape is not initiated")
    }
    TRC_FUNCTION_LEAVE("");
    return retval;
  }

  void DuktapeStuff::call(const std::string& functionName, const std::string& par, std::string& ret)
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
        THROW_EXC_TRC_WAR(std::logic_error, "Error: " << PAR(ret) << PAR(err));
        //TRC_WARNING("Error: " << PAR(ret) << PAR(err));
      }

    }
    else {
      THROW_EXC_TRC_WAR(std::logic_error, "Cannot find function");
      //TRC_WARNING("Cannot find function")
    }
    
    duk_pop_n(m_ctx, m_relativeStack);

    TRC_FUNCTION_LEAVE("");
  }

}
