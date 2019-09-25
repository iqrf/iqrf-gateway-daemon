#pragma once

#include "ComBase.h"

namespace iqrf {

  class ComAutonetwork : public ComBase
  {
  public:
    ComAutonetwork() = delete;
    ComAutonetwork( rapidjson::Document& doc ) : ComBase( doc )
    {
      parse( doc );
    }

    virtual ~ComAutonetwork()
    {
    }

    bool isSetWaves() const {
      return m_isSetWaves;
    }

    bool isSetEmptyWaves() const {
      return m_isSetEmptyWaves;
    }

    const int getActionRetries() const
    {
      return m_actionRetries;
    }

    const int getDiscoveryTxPower() const
    {
      return m_discoveryTxPower;
    }

    const bool getDiscoveryBeforeStart() const
    {
      return m_discoveryBeforeStart;
    }

    const int getWaves() const
    {
      return m_waves;
    }

    const int getEmptyWaves() const
    {
      return m_emptyWaves;
    }

  protected:
    void createResponsePayload( rapidjson::Document& doc, const IDpaTransactionResult2& res ) override
    {
      rapidjson::Pointer( "/data/rsp/response" ).Set( doc, encodeBinary( res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength() ) );
    }

  private:
    bool m_isSetWaves = false;
    bool m_isSetEmptyWaves = false;

    int m_actionRetries = 1;
    int m_discoveryTxPower = 7;
    bool m_discoveryBeforeStart = false;
    int m_waves;
    int m_emptyWaves;
    
    void parseRequest( rapidjson::Document& doc )
    {
      if ( rapidjson::Value* wavesJsonVal = rapidjson::Pointer( "/data/req/actionRetries" ).Get( doc ) )
        m_actionRetries = wavesJsonVal->GetInt();

      if ( rapidjson::Value* wavesJsonVal = rapidjson::Pointer( "/data/req/discoveryTxPower" ).Get( doc ) )
      {
        m_discoveryTxPower = wavesJsonVal->GetInt();
        if ( m_discoveryTxPower > 7 )
          THROW_EXC( std::out_of_range, "discoveryTxPower out of range. " << NAME_PAR_HEX( "discoveryTxPower", m_discoveryTxPower ) );
      }

      if ( rapidjson::Value* wavesJsonVal = rapidjson::Pointer( "/data/req/discoveryBeforeStart" ).Get( doc ) )
        m_discoveryBeforeStart = wavesJsonVal->GetBool();

      if ( rapidjson::Value* wavesJsonVal = rapidjson::Pointer( "/data/req/waves" ).Get( doc ) )
      {
        m_waves = wavesJsonVal->GetInt();
        m_isSetWaves = true;
      }

      if ( rapidjson::Value* emptyWavesJsonVal = rapidjson::Pointer( "/data/req/emptyWaves" ).Get( doc ) )
      {
        m_emptyWaves = emptyWavesJsonVal->GetInt();
        m_isSetEmptyWaves = true;
      }
    }

    // Parses document into data fields
    void parse( rapidjson::Document& doc )
    {
      parseRequest( doc );
    }
  };
}
