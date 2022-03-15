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
#include <bitset>

namespace iqrf {

  // Autonetwork input paramaters
  typedef struct
  {
    uint8_t discoveryTxPower;
    bool discoveryBeforeStart;
    bool skipDiscoveryEachWave;
    bool unbondUnrespondingNodes;
    bool abortOnTooManyNodesFound;
    bool skipPrebonding;
    uint8_t actionRetries;
    struct
    {
      std::basic_string<uint8_t> addressSpace;
      std::bitset<MAX_ADDRESS + 1> addressSpaceBitmap;
      std::map<uint32_t, uint8_t> midList;
      bool midFiltering;
      int duplicitAddressSpace;
      int duplicitMidMidList;
      int duplicitAddressMidList;
      struct
      {
        uint8_t networks;
        uint8_t network;
      }overlappingNetworks;
    }bondingControl;
    std::vector<uint16_t> hwpidFiltering;
    struct
    {
      uint8_t totalWaves;
      uint8_t emptyWaves;
      uint8_t numberOfTotalNodes;
      uint8_t numberOfNewNodes;
    }stopConditions;
  }TAutonetworkInputParams;

  class ComAutonetwork : public ComBase
  {
  public:
    ComAutonetwork() = delete;
    explicit ComAutonetwork(rapidjson::Document& doc) : ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComAutonetwork()
    {
    }

    const TAutonetworkInputParams getAutonetworkParams() const
    {
      return m_autonetworkParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }

  private:
    TAutonetworkInputParams m_autonetworkParams;

    // Parse autonetwork service parameters
    void parseRequest(rapidjson::Document& doc)
    {
      rapidjson::Value* jsonValue;

      // discoveryTxPower
      m_autonetworkParams.discoveryTxPower = 7;
      if ((jsonValue = rapidjson::Pointer("/data/req/discoveryTxPower").Get(doc)))
      {
        uint32_t txPower = jsonValue->GetInt();
        if (txPower > 7)
          txPower = 7;
        m_autonetworkParams.discoveryTxPower = (uint8_t)txPower;
      }

      // discoveryBeforeStart
      if ((jsonValue = rapidjson::Pointer("/data/req/discoveryBeforeStart").Get(doc)))
        m_autonetworkParams.discoveryBeforeStart = jsonValue->GetBool();
      else
        m_autonetworkParams.discoveryBeforeStart = false;

      // skipDiscoveryEachWave
      if ((jsonValue = rapidjson::Pointer("/data/req/skipDiscoveryEachWave").Get(doc)))
        m_autonetworkParams.skipDiscoveryEachWave = jsonValue->GetBool();
      else
        m_autonetworkParams.skipDiscoveryEachWave = false;

      // unbondUnrespondingNodes
      if ((jsonValue = rapidjson::Pointer("/data/req/unbondUnrespondingNodes").Get(doc)))
        m_autonetworkParams.unbondUnrespondingNodes = jsonValue->GetBool();
      else
        m_autonetworkParams.unbondUnrespondingNodes = false;

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

      // addressSpace
      m_autonetworkParams.bondingControl.addressSpace.clear();
      m_autonetworkParams.bondingControl.addressSpaceBitmap.reset();
      m_autonetworkParams.bondingControl.duplicitAddressSpace = 0;
      if ((jsonValue = rapidjson::Pointer("/data/req/addressSpace").Get(doc)))
      {
        const auto val = jsonValue->GetArray();
        for (auto itr = val.Begin(); itr != val.End(); ++itr)
        {
          if (itr->IsNull() == false)
          {
            // Get and validate address
            int deviceAddr = itr->GetUint();           
            if ((deviceAddr > 0) && (deviceAddr <= MAX_ADDRESS))
            {
              // Check duplicit entries
              if (m_autonetworkParams.bondingControl.addressSpaceBitmap[(uint8_t)deviceAddr] == false)
              {
                m_autonetworkParams.bondingControl.addressSpace.push_back((uint8_t)deviceAddr);
                m_autonetworkParams.bondingControl.addressSpaceBitmap[(uint8_t)deviceAddr] = true;
              }
              else
                m_autonetworkParams.bondingControl.duplicitAddressSpace++;
            }
          }
        }
      }

      // midList
      m_autonetworkParams.bondingControl.midList.clear();
      m_autonetworkParams.bondingControl.duplicitMidMidList = 0;
      m_autonetworkParams.bondingControl.duplicitAddressMidList = 0;
      if ((jsonValue = rapidjson::Pointer("/data/req/midList").Get(doc)))
      {
        const auto val = jsonValue->GetArray();
        for (auto itr = val.Begin(); itr != val.End(); ++itr)
        {
          if (itr->IsNull() == false)
          {
            // Get MID and validate deviceMid
            const auto midListObj = itr->GetObject();
			std::stringstream ss;
			std::string strDeviceMid = midListObj["deviceMID"].GetString();
			ss << std::hex << strDeviceMid;
			uint32_t deviceMid = 0;
			ss >> (uint32_t)deviceMid;
            if (deviceMid != 0)
            {
              // Check duplicid MID entries
              if (m_autonetworkParams.bondingControl.midList.find(deviceMid) == m_autonetworkParams.bondingControl.midList.end())
              {
                // Get address and validate deviceAddr
                int deviceAddr = 0;
                if (midListObj["deviceAddr"].IsNull() == false)
                {
                  int addr = midListObj["deviceAddr"].GetInt();
                  if ((addr > 0) && (addr <= MAX_ADDRESS))
                  {
                    deviceAddr = addr;
                    // Check duplicit deviceAddr in MID list
                    for (auto midListEntry : m_autonetworkParams.bondingControl.midList)
                    {
                      if (addr == midListEntry.second)
                        m_autonetworkParams.bondingControl.duplicitAddressMidList++;
                    }
                  }
                }
                m_autonetworkParams.bondingControl.midList.insert(std::make_pair(deviceMid, (uint8_t)deviceAddr));
              }
              else
                m_autonetworkParams.bondingControl.duplicitMidMidList++;
            }
          }
        }
      }

      // midFiltering
      if ((jsonValue = rapidjson::Pointer("/data/req/midFiltering").Get(doc)))
        m_autonetworkParams.bondingControl.midFiltering = jsonValue->GetBool();
      else
        m_autonetworkParams.unbondUnrespondingNodes = false;

      // overlappingNetworks/networks
      if ((jsonValue = rapidjson::Pointer("/data/req/overlappingNetworks/networks").Get(doc)))
        m_autonetworkParams.bondingControl.overlappingNetworks.networks = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.bondingControl.overlappingNetworks.networks = 0;

      // overlappingNetworks/network
      if ((jsonValue = rapidjson::Pointer("/data/req/overlappingNetworks/network").Get(doc)))
        m_autonetworkParams.bondingControl.overlappingNetworks.network = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.bondingControl.overlappingNetworks.network = 0;

      // hwpidFiltering
      m_autonetworkParams.hwpidFiltering.clear();
      if ((jsonValue = rapidjson::Pointer("/data/req/hwpidFiltering").Get(doc)))
      {
        const auto val = jsonValue->GetArray();
        for (auto itr = val.Begin(); itr != val.End(); ++itr)
        {
          if (itr->IsNull() == false)
            m_autonetworkParams.hwpidFiltering.push_back((uint16_t)itr->GetUint());
        }
      }

      // stopConditions/waves
      if ((jsonValue = rapidjson::Pointer("/data/req/stopConditions/waves").Get(doc)))
        m_autonetworkParams.stopConditions.totalWaves = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.totalWaves = 0;

      // stopConditions/emptyWaves
      if ((jsonValue = rapidjson::Pointer("/data/req/stopConditions/emptyWaves").Get(doc)))
        m_autonetworkParams.stopConditions.emptyWaves = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.emptyWaves = 1;

      // stopConditions/numberOfTotalNodes
      if ((jsonValue = rapidjson::Pointer("/data/req/stopConditions/numberOfTotalNodes").Get(doc)))
        m_autonetworkParams.stopConditions.numberOfTotalNodes = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.numberOfTotalNodes = 0;

      // stopConditions/numberOfNewNodes
      if ((jsonValue = rapidjson::Pointer("/data/req/stopConditions/numberOfNewNodes").Get(doc)))
        m_autonetworkParams.stopConditions.numberOfNewNodes = (uint8_t)jsonValue->GetInt();
      else
        m_autonetworkParams.stopConditions.numberOfNewNodes = 0;

	  // abortOnTooManyNodesFound
	  if ((jsonValue = rapidjson::Pointer("/data/req/stopConditions/abortOnTooManyNodesFound").Get(doc)))
		  m_autonetworkParams.abortOnTooManyNodesFound = jsonValue->GetBool();
	  else
		  m_autonetworkParams.abortOnTooManyNodesFound = false;
    }

    // Parses document into data fields
    void parse(rapidjson::Document& doc)
    {
      parseRequest(doc);
    }
  };
}
