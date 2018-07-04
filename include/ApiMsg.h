#pragma once

#include "rapidjson/pointer.h"

namespace iqrf {
  class ApiMsg
  {
  public:
    ApiMsg() {}
    ApiMsg(const rapidjson::Document& doc)
    {
      using namespace rapidjson;
      m_mType = Pointer("/mType").Get(doc)->GetString();
      m_msgId = Pointer("/data/msgId").Get(doc)->GetString();
      const Value* v = Pointer("/data/returnVerbose").Get(doc);
      if (v && v->IsBool()) {
        m_verbose = v->GetBool();
      }
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
      rapidjson::Pointer("/mType").Set(doc, m_mType);
      rapidjson::Pointer("/data/msgId").Set(doc, m_msgId);
    }

  private:
    std::string m_mType;
    std::string m_msgId;
    bool m_verbose = false;
  };
}
