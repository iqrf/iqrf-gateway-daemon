#pragma once

#include "ComBase.h"

namespace iqrf {

  // BackupService input paramaters
  typedef struct
  {
    uint16_t deviceAddress;
    std::string data;
    bool restartCoodinator;
  }TRestoreInputParams;

  class ComRestore : public ComBase
  {
  public:
    ComRestore() = delete;
    ComRestore( rapidjson::Document& doc ) : ComBase( doc )
    {
      parse( doc );
    }

    virtual ~ComRestore()
    {
    }

    const TRestoreInputParams getRestoreParams() const
    {
      return m_restoreParams;
    }

  protected:
    void createResponsePayload( rapidjson::Document& doc, const IDpaTransactionResult2& res ) override
    {
      rapidjson::Pointer( "/data/rsp/response" ).Set( doc, encodeBinary( res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength() ) );
    }

  private:    
    TRestoreInputParams m_restoreParams;

    // Parses document into data fields
    void parse( rapidjson::Document& doc )
    {
      rapidjson::Value* jsonValue;

      // deviceAddress
      m_restoreParams.deviceAddress = COORDINATOR_ADDRESS;
      if (jsonValue = rapidjson::Pointer("/data/req/deviceAddr").Get(doc))
      {
        uint32_t addr = jsonValue->GetInt();
        if (addr < MAX_ADDRESS)
          m_restoreParams.deviceAddress = (uint16_t)addr;
      }

      // data
      if (jsonValue = rapidjson::Pointer("/data/req/data").Get(doc))
      {
        m_restoreParams.data = jsonValue->GetString();
      }

      // restartCoordinator
      if (jsonValue = rapidjson::Pointer("/data/req/restartCoordinator").Get(doc))
        m_restoreParams.restartCoodinator = jsonValue->GetBool();
      else
        m_restoreParams.restartCoodinator = false;
    }
  };
}
