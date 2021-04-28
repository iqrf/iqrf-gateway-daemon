#pragma once

#include "ComBase.h"
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

namespace iqrf {

  // Ping input paramaters
  typedef struct
  {
    uint16_t hwpId = HWPID_DoNotCheck;
    int repeat = 1;
  }TPingInputParams;

  class ComIqmeshNetworkPing : public ComBase
  {
  public:
    ComIqmeshNetworkPing() = delete;
    explicit ComIqmeshNetworkPing(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }


    virtual ~ComIqmeshNetworkPing()
    {
    }

    const TPingInputParams getPingInputParams() const
    {
      return m_PingParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    TPingInputParams m_PingParams;

    // parses document into data fields
    void parse(rapidjson::Document& doc) 
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc)))
        m_PingParams.repeat = jsonVal->GetInt();

      // HWPID
      if ((jsonVal = rapidjson::Pointer("/data/req/hwpId").Get(doc)))
        m_PingParams.hwpId = (uint16_t)jsonVal->GetInt();
    }
  };
}
