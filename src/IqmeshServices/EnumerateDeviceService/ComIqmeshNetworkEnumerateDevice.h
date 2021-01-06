#pragma once

#include "ComBase.h"
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

namespace iqrf {

  // SmartConnect input parameters
  typedef struct TEnumerateDeviceInputParams
  {
    TEnumerateDeviceInputParams()
    {
      deviceAddress = 0;
      morePeripheralsInfo = false;
      repeat = 1;
    }
    uint16_t deviceAddress;
    bool morePeripheralsInfo = false;
    int repeat;
  }TEnumerateDeviceInputParams;

  class ComIqmeshNetworkEnumerateDevice : public ComBase
  {
  public:
    ComIqmeshNetworkEnumerateDevice() = delete;
    explicit ComIqmeshNetworkEnumerateDevice(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkEnumerateDevice()
    {
    }

    const TEnumerateDeviceInputParams getEnumerateDevicefParams() const
    {
      return m_enumerateDeviceInputParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    TEnumerateDeviceInputParams m_enumerateDeviceInputParams;

    // Parses document into data fields
    void parse(rapidjson::Document& doc) 
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc)))
        m_enumerateDeviceInputParams.repeat = jsonVal->GetInt();

      // Device address
      if ((jsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)))
        m_enumerateDeviceInputParams.deviceAddress = (uint16_t)jsonVal->GetInt();

      // morePeripheralsInfo
      if ((jsonVal = rapidjson::Pointer("/data/req/morePeripheralsInfo").Get(doc)))
        m_enumerateDeviceInputParams.morePeripheralsInfo = jsonVal->GetBool();
    }
  };
}
