#pragma once

#include "ComBase.h"

namespace iqrf {
  
  // SmartConnecy input parameters
  typedef struct TBondNodetInputParams
  {
    TBondNodetInputParams()
    {
      bondingMask = 255;
      bondingTestRetries = 1;
      repeat = 1;
    }
    uint16_t deviceAddress;
    int bondingMask;
    int bondingTestRetries;
    int repeat;
  }TBondNodetInputParams;

  class ComIqmeshNetworkBondNodeLocal : public ComBase
  {
  public:
    ComIqmeshNetworkBondNodeLocal() = delete;
    explicit ComIqmeshNetworkBondNodeLocal(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkBondNodeLocal()
    {
    }

    const TBondNodetInputParams getBondNodeInputParams() const
    {
      return m_bondNodeInputParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    TBondNodetInputParams m_bondNodeInputParams;

    // parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc)))
        m_bondNodeInputParams.repeat = jsonVal->GetInt();

      // Device address
      if (jsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc))
        m_bondNodeInputParams.deviceAddress = (uint16_t)jsonVal->GetInt();

      // bondingMak
      if (jsonVal = rapidjson::Pointer("/data/req/bondingMak").Get(doc))
        m_bondNodeInputParams.bondingMask = jsonVal->GetInt();

      // bondingTestRetries
      if (jsonVal = rapidjson::Pointer("/data/req/bondingTestRetries").Get(doc))
        m_bondNodeInputParams.bondingTestRetries = jsonVal->GetInt();
    }
  };
}
