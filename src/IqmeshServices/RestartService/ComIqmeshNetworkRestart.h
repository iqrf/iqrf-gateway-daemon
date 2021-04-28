#pragma once

#include "ComBase.h"
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

namespace iqrf {

  // Restart input paramaters
  typedef struct
  {
    uint16_t hwpId = HWPID_DoNotCheck;
    int repeat = 1;
  }TRestartInputParams;

  class ComIqmeshNetworkRestart : public ComBase
  {
  public:
    ComIqmeshNetworkRestart() = delete;
    explicit ComIqmeshNetworkRestart(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }


    virtual ~ComIqmeshNetworkRestart()
    {
    }

    const TRestartInputParams getRestartInputParams() const
    {
      return m_RestartParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    TRestartInputParams m_RestartParams;

    // parses document into data fields
    void parse(rapidjson::Document& doc) 
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc)))
        m_RestartParams.repeat = jsonVal->GetInt();

      // HWPID
      if ((jsonVal = rapidjson::Pointer("/data/req/hwpId").Get(doc)))
        m_RestartParams.hwpId = (uint16_t)jsonVal->GetInt();
    }
  };
}
