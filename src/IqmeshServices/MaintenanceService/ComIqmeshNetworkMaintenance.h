#pragma once

#include "ComBase.h"
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

namespace iqrf {

  // Maintenance input paramaters
  typedef struct
  {
    uint16_t deviceAddr = COORDINATOR_ADDRESS;
    uint8_t RFchannel = 0;
    uint8_t RFfilter = 0;
    IDpaTransaction2::FrcResponseTime measurementTime = IDpaTransaction2::FrcResponseTime::k40Ms;
    int measurementTimeMS = 40;
    int repeat = 1;
  }TMaintenanceInputParams;

  class ComIqmeshNetworkMaintenance : public ComBase
  {
  public:
    ComIqmeshNetworkMaintenance() = delete;
    explicit ComIqmeshNetworkMaintenance(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }


    virtual ~ComIqmeshNetworkMaintenance()
    {
    }

    const TMaintenanceInputParams getMaintenanceInputParams() const
    {
      return m_MaintenanceParams;
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    TMaintenanceInputParams m_MaintenanceParams;

    // parses document into data fields
    void parse(rapidjson::Document& doc) 
    {
      rapidjson::Value* jsonVal;

      // Repeat
      if ((jsonVal = rapidjson::Pointer("/data/repeat").Get(doc)))
        m_MaintenanceParams.repeat = jsonVal->GetInt();

      // deviceAddr
      if ((jsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)))
      {
        m_MaintenanceParams.deviceAddr = (uint16_t)jsonVal->GetInt();
        if (m_MaintenanceParams.deviceAddr != COORDINATOR_ADDRESS && m_MaintenanceParams.deviceAddr != BROADCAST_ADDRESS)
          m_MaintenanceParams.deviceAddr = COORDINATOR_ADDRESS;
      }

      // RFchannel
      if ((jsonVal = rapidjson::Pointer("/data/req/RFchannel").Get(doc)))
      {
        m_MaintenanceParams.RFchannel = (uint8_t)jsonVal->GetInt();
        if (m_MaintenanceParams.RFchannel > 67)
          m_MaintenanceParams.RFchannel = 0;
      }

      // RFfilter
      if ((jsonVal = rapidjson::Pointer("/data/req/RFfilter").Get(doc)))
      {
        m_MaintenanceParams.RFfilter = (uint8_t)jsonVal->GetInt();
        if (m_MaintenanceParams.RFfilter > 64 && m_MaintenanceParams.RFfilter != 255)
          m_MaintenanceParams.RFfilter = 0;
      }

      // measurementTime
      if ((jsonVal = rapidjson::Pointer("/data/req/measurementTime").Get(doc)))
      {
        m_MaintenanceParams.measurementTimeMS = jsonVal->GetInt();
        switch (m_MaintenanceParams.measurementTimeMS)
        {
        case 360:
          m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k360Ms;
          break;
        case 680:
          m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k680Ms;
          break;
        case 1320:
          m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k1320Ms;
          break;
        case 2600:
          m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k2600Ms;
          break;
        case 5160:
          m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k5160Ms;
          break;
        case 10280:
          m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k10280Ms;
          break;
        case 20620:
          m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k20620Ms;
          break;
        default:
          m_MaintenanceParams.measurementTime = IDpaTransaction2::FrcResponseTime::k40Ms;
          break;
        }        
      }
    }
  };
}
