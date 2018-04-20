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

    const std::vector<int> getDeviceAddr() const
    {
      return m_deviceAddr;
    }

    

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    bool m_isSetDeviceAddr = false;

    int m_repeat = 1;
    std:: vector<int> m_deviceAddr;


    void parseRepeat(rapidjson::Document& doc) {
      if (rapidjson::Pointer("/data/repeat").IsValid()) {
        m_repeat = rapidjson::Pointer("/data/repeat").Get(doc)->GetInt();
      }
    }

    void parseRequest(rapidjson::Document& doc) {
      if (rapidjson::Pointer("/data/req/deviceAddr").IsValid()) {
        rapidjson::Value* reqVal = rapidjson::Pointer("/data/req").Get(doc);
        m_deviceAddr = jutils::getMemberAsVector<int>("deviceAddr", *reqVal);
        m_isSetDeviceAddr = true;
      }
    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRepeat(doc);
      parseRequest(doc);
    }
  };
}
