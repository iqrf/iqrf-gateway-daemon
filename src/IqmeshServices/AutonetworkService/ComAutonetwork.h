#pragma once

#include "ComBase.h"

namespace iqrf {
  
  class ComAutonetwork : public ComBase
  {
  public:
    ComAutonetwork() = delete;
    ComAutonetwork(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComAutonetwork()
    {
    }

    bool isSetWaves() const {
      return m_isSetWaves;
    }

    const int getWaves() const
    {
      return m_waves;
    }

    bool isSetEmptyWaves() const {
      return m_isSetEmptyWaves;
    }

    const int getEmptyWaves() const
    {
      return m_emptyWaves;
    }

    const int getDiscoveryTxPower() const
    {
      return m_discoveryTxPower;
    }


  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    bool m_isSetWaves = false;
    bool m_isSetEmptyWaves = false;

    int m_waves;
    int m_emptyWaves;
    int m_discoveryTxPower = 1;


    void parseRequest(rapidjson::Document& doc) {
      if (rapidjson::Value* wavesJsonVal = rapidjson::Pointer("/data/req/waves").Get(doc)) {
        m_waves = wavesJsonVal->GetInt();
      }
      m_isSetWaves = true;

      if (rapidjson::Value* emptyWavesJsonVal = rapidjson::Pointer("/data/req/emptyWaves").Get(doc)) {
        m_emptyWaves = emptyWavesJsonVal->GetInt();
      }
      m_isSetEmptyWaves = true;
      
      if (rapidjson::Value* discoveryTxPowerJsonVal = rapidjson::Pointer("/data/req/discoveryTxPower").Get(doc)) {
        m_discoveryTxPower = discoveryTxPowerJsonVal->GetInt();
      }
    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRequest(doc);
    }

  };
}
