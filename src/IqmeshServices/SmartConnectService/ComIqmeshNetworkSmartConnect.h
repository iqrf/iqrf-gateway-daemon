#pragma once

#include "ComBase.h"
#include "Trace.h"
#include <list>

namespace iqrf {
  class ComIqmeshNetworkSmartConnect : public ComBase
  {
  public:
    ComIqmeshNetworkSmartConnect() = delete;
    ComIqmeshNetworkSmartConnect(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkSmartConnect()
    {
    }

    int getRepeat() const {
      return m_repeat;
    }

    bool isSetDeviceAddr() const {
      return m_isSetDeviceAddr;
    }

    int getDeviceAddr() const
    {
      return m_deviceAddr;
    }

    int getBondingTestRetries() const
    {
      return m_bondingTestRetries;
    }

    bool isSetSmartConnectCode() const {
      return m_isSetSmartConnectCode;
    }

    const std::string getSmartConnectCode() const
    {
      return m_smartConnectCode;
    }

    bool isSetUserData() const {
      return m_isSetUserData;
    }

    const std::basic_string<uint8_t> getUserData() const
    {
      return m_userData;
    }


  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    bool m_isSetDeviceAddr = false;
    bool m_isSetSmartConnectCode = false;
    bool m_isSetUserData = false;

    int m_repeat = 1;

    int m_deviceAddr;
    int m_bondingTestRetries = 1;
    std::string m_smartConnectCode;
    std::basic_string<uint8_t> m_userData;


    void parseRepeat(rapidjson::Document& doc) {
      if (rapidjson::Value* repeatJsonVal = rapidjson::Pointer("/data/repeat").Get(doc)) {
        m_repeat = repeatJsonVal->GetInt();
      }
    }

    void parseRequest(rapidjson::Document& doc) {
      if (rapidjson::Value* deviceAddrJsonVal = rapidjson::Pointer("/data/req/deviceAddr").Get(doc)) {
        m_deviceAddr = deviceAddrJsonVal->GetInt();
        m_isSetDeviceAddr = true;
      }

      m_bondingTestRetries = rapidjson::Pointer("/data/req/bondingTestRetries").GetWithDefault(doc, 1).GetInt();
       
      if (rapidjson::Value* deviceAddrJsonVal = rapidjson::Pointer("/data/req/smartConnectCode").Get(doc)) {
        m_smartConnectCode = deviceAddrJsonVal->GetString();
        m_isSetSmartConnectCode = true;
      }

      if (rapidjson::Value* userDataJson = rapidjson::Pointer("/data/req/userData").Get(doc)) {
        if (userDataJson->IsArray()) {
          for (rapidjson::SizeType i = 0; i < userDataJson->Size(); i++) {
            m_userData.push_back((*userDataJson)[i].GetInt());
          }
          m_isSetUserData = true;
        }
        else {
          THROW_EXC(std::logic_error, "User data must be array.");
        }
      }
    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRepeat(doc);
      parseRequest(doc);
    }
  };
}
