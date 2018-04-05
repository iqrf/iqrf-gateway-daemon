#pragma once

#include "duktape.h"
#include "IJsCacheService.h"
#include "rapidjson/document.h"
#include <map>
#include <condition_variable>
#include <thread>
#include <mutex>

namespace iqrf {
  class DuktapeStuff
  {
  public:
    DuktapeStuff();
    ~DuktapeStuff();
    void init(const std::map<int, const IJsCacheService::StdDriver*>& scripts);
    void finit();
    void call(const std::string& functionName, const std::string& par, std::string& ret);
  private:
    bool findFunc(const std::string& functionName);

    bool m_init = false;
    duk_context *m_ctx = nullptr;
    std::thread m_thread;
    bool m_thdRunFlg = true;
    std::mutex m_mtx;
    std::condition_variable m_cond;
    bool m_doCall = false;
    std::string m_callFunctionName;
    int m_relativeStack = 0;

    void thdRun();
    
    std::map<int, const IJsCacheService::StdDriver*> m_scripts;
  };

}
