#pragma once

#include "ComBase.h"
#include "Trace.h"
#include <list>

namespace iqrf {

  // OtaUpload input parameters
  typedef struct TOtaUploadInputParams
  {
    TOtaUploadInputParams()
    {
      hwpId = HWPID_DoNotCheck;
      repeat = 1;
    }
    uint16_t deviceAddress;
    uint16_t hwpId;
    std::string fileName;
    uint16_t repeat;
    uint16_t startMemAddr;
    std::string loadingAction;
  }TOtaUploadInputParams;

  class ComIqmeshNetworkOtaUpload : public ComBase
  {
  public:
    ComIqmeshNetworkOtaUpload() = delete;
    explicit ComIqmeshNetworkOtaUpload(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkOtaUpload()
    {
    }

    const TOtaUploadInputParams getOtaUploadInputParams() const
    {
      return m_otaUploadInputParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    TOtaUploadInputParams m_otaUploadInputParams;

    // Parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc))) {
        m_otaUploadInputParams.repeat = (uint16_t)jsonVal->GetInt();
      }
        
      // Device address
      if ((jsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc))) {
        m_otaUploadInputParams.deviceAddress = (uint16_t)jsonVal->GetInt();
      }
        
      // hwpId
      if ((jsonVal = rapidjson::Pointer("/data/req/hwpId").Get(doc))) {
        m_otaUploadInputParams.hwpId = (uint16_t)jsonVal->GetInt();
      }

      // File name
      if ((jsonVal = rapidjson::Pointer("/data/req/fileName").Get(doc))) {
        m_otaUploadInputParams.fileName = jsonVal->GetString();
      }

      // Start memory address
      if ((jsonVal = rapidjson::Pointer("/data/req/startMemAddr").Get(doc))) {
        m_otaUploadInputParams.startMemAddr = (uint16_t)jsonVal->GetInt();
      }
        
      // Loading action
      if ((jsonVal = rapidjson::Pointer("/data/req/loadingAction").Get(doc))) {
        m_otaUploadInputParams.loadingAction = jsonVal->GetString();
      }
    }
  };
}