#pragma once

#include "ComBase.h"
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

namespace iqrf {
  class ComIqmeshNetworkEnumerateDevice : public ComBase
  {
  public:
    ComIqmeshNetworkEnumerateDevice() = delete;
    ComIqmeshNetworkEnumerateDevice(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkEnumerateDevice()
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
  
    const bool getMorePeripheralsInfo() {
      return m_morePeripheralsInfo;
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
    int m_deviceAddr;
    bool m_morePeripheralsInfo = false;
    

    void parseRepeat( rapidjson::Document& doc ) {
      if ( rapidjson::Value* repeatJsonVal = rapidjson::Pointer( "/data/repeat" ).Get( doc ) )
        m_repeat = repeatJsonVal->GetInt();
      if ( m_repeat < 1 || m_repeat > 10 )
        m_repeat = 1;
    }

    void parseRequest(rapidjson::Document& doc) {
      if (rapidjson::Value* deviceAddrJsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)) {
        m_deviceAddr = deviceAddrJsonVal->GetInt();
        m_isSetDeviceAddr = true;
      }

      if (rapidjson::Value* morePerInfoJsonVal = rapidjson::Pointer("/data/req/morePeripheralsInfo").Get(doc)) {
        m_morePeripheralsInfo = morePerInfoJsonVal->GetBool();
      }
    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRepeat(doc);
      parseRequest(doc);
    }
  };
}
