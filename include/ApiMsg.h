#pragma once

#include "rapidjson/pointer.h"

namespace iqrf {
  class ApiMsg
  {
  public:
    ApiMsg() {}
    ApiMsg(rapidjson::Document& doc)
    {
      m_msgId = rapidjson::Pointer("/data/msgId").Get(doc)->GetString();
      m_verbose = rapidjson::Pointer("/data/returnVerbose").GetWithDefault(doc, m_verbose).GetBool();
    }

    virtual ~ApiMsg()
    {

    }

    const std::string& getMsgId() const
    {
      return m_msgId;
    }

    const bool getVerbose() const
    {
      return m_verbose;
    }

    void createResponse(rapidjson::Document& doc)
    {
      rapidjson::Pointer("/mType").Set(doc, "unknown-mType"); // it's here just to keep order - replaced later in processing, 
      rapidjson::Pointer("/data/msgId").Set(doc, m_msgId);
    }

  private:
    std::string m_msgId;
    bool m_verbose = false;
  };
}
