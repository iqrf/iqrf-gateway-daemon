#pragma once

#include "ComBase.h"

namespace iqrf {
  
  class ComIqmeshNetworkBondNodeLocal : public ComBase
  {
  public:
    ComIqmeshNetworkBondNodeLocal() = delete;
    ComIqmeshNetworkBondNodeLocal(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkBondNodeLocal()
    {
    }

    int getRepeat() const {
      return m_repeat;
    }

    bool isSetDeviceAddr() const {
      return m_isSetDeviceAddr;
    }

    const int getDeviceAddr() const
    {
      return m_deviceAddr;
    }
    
    bool isSetBondingMask() const {
      return m_isSetBondingMask;
    }

    const int getBondingMask() const
    {
      return m_bondingMask;
    }


  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    bool m_isSetDeviceAddr = false;
    bool m_isSetBondingMask = false;

    int m_repeat = 1;
    int m_deviceAddr;
    int m_bondingMask = 0x00;


    void parseRepeat(rapidjson::Document& doc) {
      if (rapidjson::Value* repeatJsonVal = rapidjson::Pointer("/data/repeat").Get(doc)) {
        m_repeat = repeatJsonVal->GetInt();
      }
    }

    void parseRequest(rapidjson::Document& doc) {
      if (rapidjson::Value* repeatJsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)) {
        m_deviceAddr = repeatJsonVal->GetInt();
      }
      m_isSetDeviceAddr = true;

      if ( rapidjson::Value* repeatJsonVal = rapidjson::Pointer( "/data/req/bondingMask" ).Get( doc ) ) {
        m_bondingMask = repeatJsonVal->GetInt();
      }
      m_isSetBondingMask = true;
    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRepeat(doc);
      parseRequest(doc);
    }

  };
}
