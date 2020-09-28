#pragma once

#include "ComBase.h"
#include <vector>

namespace iqrf {

  // RemoveService input paramaters
  typedef struct
  {
    uint8_t deviceAddr = 0;
    uint16_t hwpId = 0xffff;
    bool wholeNetwork = false;
    int repeat = 1;
    std::basic_string<uint8_t> deviceAddrList;
    bool clearAllBonds = false;
  }TRemoveBondInputParams;

  class ComIqmeshNetworkRemoveBond : public ComBase
  {
  public:
    ComIqmeshNetworkRemoveBond() = delete;
    explicit ComIqmeshNetworkRemoveBond(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkRemoveBond()
    {
    }

    const TRemoveBondInputParams getRomveBondParams() const
    {
      return m_removeBondInputParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    TRemoveBondInputParams m_removeBondInputParams;

    // parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonValue;

      // deviceAddress
      if (jsonValue = rapidjson::Pointer("/data/req/deviceAddr").Get(doc))
      {
        m_removeBondInputParams.deviceAddrList.clear();

        if (jsonValue->IsInt())
        {
          uint32_t addr = jsonValue->GetInt();
          m_removeBondInputParams.deviceAddr = (uint8_t)addr;
        }
        
        if (jsonValue->IsArray())
        {
          for (auto itr = jsonValue->Begin(); itr != jsonValue->End(); ++itr)
          {
            if (itr->IsInt())
            {
              uint8_t addr = (uint8_t)itr->GetInt();
              m_removeBondInputParams.deviceAddrList.push_back(addr);
            }
          }
        }
      }

      // hwpId
      if (jsonValue = rapidjson::Pointer("/data/req/hwpId").Get(doc))
        m_removeBondInputParams.hwpId = (uint16_t)jsonValue->GetInt();

      // wholeNetwork
      if (jsonValue = rapidjson::Pointer("/data/req/wholeNetwork").Get(doc))
        m_removeBondInputParams.wholeNetwork = jsonValue->GetBool();

      // clearAllBonds
      if (jsonValue = rapidjson::Pointer("/data/req/clearAllBonds").Get(doc))
        m_removeBondInputParams.clearAllBonds = jsonValue->GetBool();

      // repeat
      if (rapidjson::Value* repeatJsonVal = rapidjson::Pointer("/data/repeat").Get(doc))
        m_removeBondInputParams.repeat = repeatJsonVal->GetInt();
    }
  };
}
