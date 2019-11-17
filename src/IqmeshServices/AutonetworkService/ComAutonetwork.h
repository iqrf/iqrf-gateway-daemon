#pragma once

#include "ComBase.h"

namespace iqrf {

  // Autonetwork input paramaters
  typedef struct
  {
    uint8_t discoveryTxPower;
    bool discoveryBeforeStart;
    uint8_t actionRetries;
    struct 
    {
      uint8_t networks;
      uint8_t network;
    }overlappingNetworks;
    struct
    {
      bool active;
      uint16_t hwpid[256];
    }hwpidFiltering;
    struct 
    {
      uint8_t waves;
      uint8_t networkSize;
      uint8_t emptyWaves;
    }stopConditions;
  }TAutonetworkInputParams;

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

    const TAutonetworkInputParams getAnutonetworkParams() const
    {
      return m_autonetworkParams;
    }

  protected:
    void createResponsePayload( rapidjson::Document& doc, const IDpaTransactionResult2& res ) override
    {
      rapidjson::Pointer( "/data/rsp/response" ).Set( doc, encodeBinary( res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength() ) );
    }

  private:
    TAutonetworkInputParams m_autonetworkParams;
    
    // Parse autonetwork service parameters
    void parseRequest( rapidjson::Document& doc )
    {
      rapidjson::Value* jsonValue;

      // discoveryTxPower
      m_autonetworkParams.discoveryTxPower = 7;
      if ( jsonValue = rapidjson::Pointer( "/data/req/discoveryTxPower" ).Get( doc ) )
      {
        uint32_t txPower = jsonValue->GetInt();
        if ( txPower > 7 )
          txPower = 7;
        m_autonetworkParams.discoveryTxPower = txPower;
      }

      // discoveryBeforeStart
      if ( jsonValue = rapidjson::Pointer( "/data/req/discoveryBeforeStart" ).Get( doc ) )
        m_autonetworkParams.discoveryBeforeStart = jsonValue->GetBool();
      else
        m_autonetworkParams.discoveryBeforeStart = false;

      // actionRetries
      if ( jsonValue = rapidjson::Pointer( "/data/req/actionRetries" ).Get( doc ) )
        m_autonetworkParams.actionRetries = jsonValue->GetInt();
      else
        m_autonetworkParams.actionRetries = 1;

      // overlappingNetworks/networks
      if ( jsonValue = rapidjson::Pointer( "/data/req/overlappingNetworks/networks" ).Get( doc ) )
        m_autonetworkParams.overlappingNetworks.networks = jsonValue->GetInt();
      else
        m_autonetworkParams.overlappingNetworks.networks = 0;

      // overlappingNetworks/network
      if ( jsonValue = rapidjson::Pointer( "/data/req/overlappingNetworks/network" ).Get( doc ) )
        m_autonetworkParams.overlappingNetworks.network = jsonValue->GetInt();
      else
        m_autonetworkParams.overlappingNetworks.network = 0;

      // hwpidFiltering
      if ( jsonValue = rapidjson::Pointer( "/data/req/hwpidFiltering" ).Get( doc ) )
      {
        //m_autonetworkParams.hwpidFiltering.hwpid = jsonValue->GetArray();
        m_autonetworkParams.hwpidFiltering.active = true;
      }
      else
        m_autonetworkParams.hwpidFiltering.active = false;

      // stopConditions/waves
      if ( jsonValue = rapidjson::Pointer( "/data/req/stopConditions/waves" ).Get( doc ) )
        m_autonetworkParams.stopConditions.waves = jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.waves = 0;

      // stopConditions/emptyWaves
      if ( jsonValue = rapidjson::Pointer( "/data/req/stopConditions/emptyWaves" ).Get( doc ) )
        m_autonetworkParams.stopConditions.emptyWaves = jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.emptyWaves = 1;

      // stopConditions/networkSize
      if ( jsonValue = rapidjson::Pointer( "/data/req/stopConditions/networkSize" ).Get( doc ) )
        m_autonetworkParams.stopConditions.networkSize = jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.networkSize = 0;
    }

    // Parses document into data fields
    void parse( rapidjson::Document& doc )
    {
      parseRequest( doc );
    }
  };
}
