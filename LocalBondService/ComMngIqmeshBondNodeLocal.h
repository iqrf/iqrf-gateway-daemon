#pragma once

#include "ComBase.h"

namespace iqrf {
  
  class ComMngIqmeshBondNodeLocal : public ComBase
  {
  public:
    ComMngIqmeshBondNodeLocal() = delete;
    ComMngIqmeshBondNodeLocal(rapidjson::Document& doc)
      :ComBase(doc)
    {
      m_repeat = rapidjson::Pointer("/data/repeat").GetWithDefault(doc, (int)m_repeat).GetInt();
      parseHexaNum(m_deviceAddr, rapidjson::Pointer("/data/req/deviceAddr").Get(doc)->GetString());
    }

    virtual ~ComMngIqmeshBondNodeLocal()
    {
    }

    const uint32_t getRepeat() const
    {
      return m_repeat;
    }
    
    const uint16_t getDeviceAddr() const
    {
      return m_deviceAddr;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    uint32_t m_repeat;
    uint16_t m_deviceAddr;
  };
}
