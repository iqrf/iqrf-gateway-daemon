/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "ComBase.h"

namespace iqrf {

  // Autonetwork input paramaters
  typedef struct
  {
    uint8_t discoveryTxPower;
    bool discoveryBeforeStart;
    bool skipDiscoveryEachWave;
    bool unbondUnrespondingNodes;
    bool skipPrebonding;
    uint8_t actionRetries;
    struct 
    {
      uint8_t networks;
      uint8_t network;
    }overlappingNetworks;
    std::vector<uint16_t> hwpidFiltering;
    struct 
    {
      uint8_t waves;
      uint8_t emptyWaves;
      uint8_t numberOfTotalNodes;
      uint8_t numberOfNewNodes;
      bool abortOnTooManyNodesFound;
    }stopConditions;
  }TAutonetworkInputParams;

  class ComAutonetwork : public ComBase
  {
  public:
    ComAutonetwork() = delete;
    explicit ComAutonetwork( rapidjson::Document& doc ) : ComBase( doc )
    {
      parse( doc );
    }

    virtual ~ComAutonetwork()
    {
    }

    const TAutonetworkInputParams getAutonetworkParams() const
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
      if ( (jsonValue = rapidjson::Pointer( "/data/req/discoveryTxPower" ).Get( doc )) )
      {
        uint32_t txPower = jsonValue->GetInt();
        if ( txPower > 7 )
          txPower = 7;
        m_autonetworkParams.discoveryTxPower = (uint8_t)txPower;
      }

      // discoveryBeforeStart
      if ( (jsonValue = rapidjson::Pointer( "/data/req/discoveryBeforeStart" ).Get( doc )) )
        m_autonetworkParams.discoveryBeforeStart = jsonValue->GetBool();
      else
        m_autonetworkParams.discoveryBeforeStart = false;

      // skipDiscoveryEachWave
      if ( (jsonValue = rapidjson::Pointer( "/data/req/skipDiscoveryEachWave" ).Get( doc )) )
        m_autonetworkParams.skipDiscoveryEachWave = jsonValue->GetBool();
      else
        m_autonetworkParams.skipDiscoveryEachWave = false;

      // unbondUnrespondingNodes
      if ((jsonValue = rapidjson::Pointer("/data/req/unbondUnrespondingNodes").Get(doc)))
        m_autonetworkParams.unbondUnrespondingNodes = jsonValue->GetBool();
      else
        m_autonetworkParams.unbondUnrespondingNodes = true;

      // skipPrebonding
      if ((jsonValue = rapidjson::Pointer("/data/req/skipPrebonding").Get(doc)))
        m_autonetworkParams.skipPrebonding = jsonValue->GetBool();
      else
        m_autonetworkParams.skipPrebonding = false;

      // actionRetries
      if ((jsonValue = rapidjson::Pointer("/data/req/actionRetries").Get(doc)))
        m_autonetworkParams.actionRetries = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.actionRetries = 1;

      // overlappingNetworks/networks
      if ( (jsonValue = rapidjson::Pointer( "/data/req/overlappingNetworks/networks" ).Get( doc )) )
        m_autonetworkParams.overlappingNetworks.networks = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.overlappingNetworks.networks = 0;

      // overlappingNetworks/network
      if ( (jsonValue = rapidjson::Pointer( "/data/req/overlappingNetworks/network" ).Get( doc )) )
        m_autonetworkParams.overlappingNetworks.network = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.overlappingNetworks.network = 0;

      // hwpidFiltering
      m_autonetworkParams.hwpidFiltering.clear();
      if ( (jsonValue = rapidjson::Pointer( "/data/req/hwpidFiltering" ).Get( doc )) )
      {
        const auto val = jsonValue->GetArray();
        for ( auto itr = val.Begin(); itr != val.End(); ++itr )
        {
          if ( itr->IsNull() == false )
            m_autonetworkParams.hwpidFiltering.push_back((uint16_t)itr->GetUint());
        }
      }

      // stopConditions/waves
      if ( (jsonValue = rapidjson::Pointer( "/data/req/stopConditions/waves" ).Get( doc )) )
        m_autonetworkParams.stopConditions.waves = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.waves = 0;

      // stopConditions/emptyWaves
      if ( (jsonValue = rapidjson::Pointer( "/data/req/stopConditions/emptyWaves" ).Get( doc )) )
        m_autonetworkParams.stopConditions.emptyWaves = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.emptyWaves = 1;

      // stopConditions/numberOfTotalNodes
      if ( (jsonValue = rapidjson::Pointer( "/data/req/stopConditions/numberOfTotalNodes" ).Get( doc )) )
        m_autonetworkParams.stopConditions.numberOfTotalNodes = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.numberOfTotalNodes = 0;

      // stopConditions/numberOfNewNodes
      if ( (jsonValue = rapidjson::Pointer( "/data/req/stopConditions/numberOfNewNodes" ).Get( doc )) )
        m_autonetworkParams.stopConditions.numberOfNewNodes = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.numberOfNewNodes = 0;

      // abortOnTooManyNodesFound
      if ( (jsonValue = rapidjson::Pointer( "/data/req/stopConditions/abortOnTooManyNodesFound" ).Get( doc )) )
        m_autonetworkParams.stopConditions.abortOnTooManyNodesFound = jsonValue->GetBool();
      else
        m_autonetworkParams.stopConditions.abortOnTooManyNodesFound = false;
    }

    // Parses document into data fields
    void parse( rapidjson::Document& doc )
    {
      parseRequest( doc );
    }
  };
}
