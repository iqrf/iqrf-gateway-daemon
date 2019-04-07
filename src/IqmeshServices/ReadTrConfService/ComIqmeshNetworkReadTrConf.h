#pragma once

#include "ComBase.h"
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

namespace iqrf {
  class ComIqmeshNetworkReadTrConf : public ComBase
  {
  public:
    ComIqmeshNetworkReadTrConf() = delete;
    ComIqmeshNetworkReadTrConf(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkReadTrConf()
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

    bool isSetHwpId() const {
      return m_isSetHwpId;
    }

    const int getHwpId() const
    {
      return m_hwpId;
    }


  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    bool m_isSetDeviceAddr = false;
    bool m_isSetHwpId = false;

    int m_repeat = 1;
    int m_deviceAddr;
    int m_hwpId;


    void parseRepeat(rapidjson::Document& doc) {
      if (rapidjson::Value* repeatJsonVal = rapidjson::Pointer("/data/repeat").Get(doc)) {
        m_repeat = repeatJsonVal->GetInt();
      }
    }

    void parseRequest(rapidjson::Document& doc) {
      if (rapidjson::Value* devAddrJsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)) {
        m_deviceAddr = devAddrJsonVal->GetInt();
        m_isSetDeviceAddr = true;
      }

      if (rapidjson::Value* hwpIdJsonVal = rapidjson::Pointer("/data/req/hwpId").Get(doc)) {
        m_hwpId = hwpIdJsonVal->GetInt();
        m_isSetHwpId = true;
      }
    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRepeat(doc);
      parseRequest(doc);
    }
  };
}
