#pragma once

#include "ComBase.h"

namespace iqrf {

  // BackupService input paramaters
  typedef struct
  {
    uint16_t deviceAddress = 0;
    bool wholeNetwork = false;
  }TBackupInputParams;

  class ComBackup : public ComBase
  {
  public:
    ComBackup() = delete;
    ComBackup( rapidjson::Document& doc ) : ComBase( doc )
    {
      parse( doc );
    }

    virtual ~ComBackup()
    {
    }

    const TBackupInputParams getBackupParams() const
    {
      return m_backupParams;
    }

  protected:
    void createResponsePayload( rapidjson::Document& doc, const IDpaTransactionResult2& res ) override
    {
      rapidjson::Pointer( "/data/rsp/response" ).Set( doc, encodeBinary( res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength() ) );
    }

  private:    
    TBackupInputParams m_backupParams;

    // Parses document into data fields
    void parse( rapidjson::Document& doc )
    {
      rapidjson::Value* jsonValue;

      // deviceAddress
      m_backupParams.deviceAddress = COORDINATOR_ADDRESS;
      if (jsonValue = rapidjson::Pointer("/data/req/deviceAddr").Get(doc))
      {
        uint32_t addr = jsonValue->GetInt();
        if (addr < MAX_ADDRESS)
          m_backupParams.deviceAddress = (uint16_t)addr;
      }

      // wholeNetwork
      if (jsonValue = rapidjson::Pointer("/data/req/wholeNetwork").Get(doc))
        m_backupParams.wholeNetwork = jsonValue->GetBool();
      else
        m_backupParams.wholeNetwork = false;
    }
  };
}
