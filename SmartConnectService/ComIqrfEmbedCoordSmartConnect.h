#pragma once

#include "ComBase.h"
#include <list>

namespace iqrf {
  class ComIqrfEmbedCoordSmartConnect : public ComBase
  {
  public:
    ComIqrfEmbedCoordSmartConnect() = delete;
    ComIqrfEmbedCoordSmartConnect(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqrfEmbedCoordSmartConnect()
    {
    }

    bool isSetTimeout() const {
      return m_isSetTimeout;
    }

    const int getTimeout() const
    {
      return m_timeout;
    }

    bool isSetNadr() const {
      return m_isSetNadr;
    }

    const int getNadr() const
    {
      return m_nadr;
    }

    bool isSetHwpId() const {
      return m_isSetHwpId;
    }

    const int getHwpId() const
    {
      return m_hwpId;
    }

    bool isSetReqAddr() const {
      return m_isSetReqAddr;
    }

    const int getReqAddr() const
    {
      return m_reqAddr;
    }

    bool isSetBondingTestRetries() const {
      return m_isSetBondingTestRetries;
    }

    const int getBondingTestRetries() const
    {
      return m_bondingTestRetries;
    }

    bool isSetIbk() const {
      return m_isSetIbk;
    }

    const std::basic_string<uint8_t> getIbk() const
    {
      return m_ibk;
    }

    bool isSetMid() const {
      return m_isSetMid;
    }

    const std::basic_string<uint8_t> getMid() const
    {
      return m_mid;
    }

    bool isSetBondingChannel() const {
      return m_isSetBondingChannel;
    }

    const int getBondingChannel() const
    {
      return m_bondingChannel;
    }

    bool isSetVirtualDeviceAddress() const {
      return m_isSetVirtualDeviceAddress;
    }

    const int getVirtualDeviceAddress() const
    {
      return m_virtualDeviceAddress;
    }

    bool isSetUserData() const {
      return m_isSetUserData;
    }

    const std::basic_string<uint8_t> getUserData() const
    {
      return m_userData;
    }

    bool isSetReturnVerbose() const {
      return m_isSetReturnVerbose;
    }

    const bool getReturnVerbose() const
    {
      return m_returnVerbose;
    }


  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    bool m_isSetTimeout = false;
    bool m_isSetNadr = false;
    bool m_isSetHwpId = false;
    bool m_isSetReqAddr = false;
    bool m_isSetBondingTestRetries = false;
    bool m_isSetIbk = false;
    bool m_isSetMid = false;
    bool m_isSetBondingChannel = false;
    bool m_isSetVirtualDeviceAddress = false;
    bool m_isSetUserData = false;
    bool m_isSetReturnVerbose = false;

    int m_timeout;
    int m_nadr;
    int m_hwpId;
    int m_reqAddr;
    int m_bondingTestRetries;
    std::basic_string<uint8_t> m_ibk;
    std::basic_string<uint8_t> m_mid;
    int m_bondingChannel;
    int m_virtualDeviceAddress;
    std::basic_string<uint8_t> m_userData;
    bool m_returnVerbose = false;


    void parseTimeout(rapidjson::Document& doc) {
      if (rapidjson::Pointer("/data/timeout").IsValid()) {
        m_timeout = rapidjson::Pointer("/data/timeout").Get(doc)->GetInt();
        m_isSetTimeout = true;
      }
    }

    void parseRequest(rapidjson::Document& doc) {
      if (rapidjson::Pointer("/data/req/nAdr").IsValid()) {
        m_nadr = rapidjson::Pointer("/data/req/nAdr").Get(doc)->GetInt();
        m_isSetNadr = true;
      }

      if (rapidjson::Pointer("/data/req/hwpId").IsValid()) {
        m_hwpId = rapidjson::Pointer("/data/req/hwpId").Get(doc)->GetInt();
        m_isSetHwpId = true;
      }

      if (rapidjson::Pointer("/data/req/reqAddr").IsValid()) {
        m_reqAddr = rapidjson::Pointer("/data/req/reqAddr").Get(doc)->GetInt();
        m_isSetReqAddr = true;
      }

      if (rapidjson::Pointer("/data/req/bondingTestRetries").IsValid()) {
        m_bondingTestRetries = rapidjson::Pointer("/data/req/bondingTestRetries").Get(doc)->GetInt();
        m_isSetBondingTestRetries = true;
      }

      if (rapidjson::Pointer("/data/req/ibk").IsValid()) {
        rapidjson::Value* ibkJson = rapidjson::Pointer("/data/req/ibk").Get(doc);
        if (ibkJson->IsArray()) {
          for (rapidjson::SizeType i = 0; i < ibkJson->Size(); i++) {
            m_ibk.push_back(ibkJson[i].GetInt());
          }
        }
        else {
          throw std::out_of_range("Invalid /data/req/ibk");
        }
        m_isSetIbk = true;
      }

      if (rapidjson::Pointer("/data/req/mid").IsValid()) {
        rapidjson::Value* midJson = rapidjson::Pointer("/data/req/mid").Get(doc);
        if (midJson->IsArray()) {
          for (rapidjson::SizeType i = 0; i < midJson->Size(); i++) {
            m_mid.push_back(midJson[i].GetInt());
          }
        }
        else {
          throw std::out_of_range("Invalid /data/req/mid");
        }
        m_isSetMid = true;
      }

      if (rapidjson::Pointer("/data/req/bondingChannel").IsValid()) {
        m_bondingChannel = rapidjson::Pointer("/data/req/bondingChannel").Get(doc)->GetInt();
        m_isSetBondingChannel = true;
      }

      if (rapidjson::Pointer("/data/req/virtualDeviceAddress").IsValid()) {
        m_virtualDeviceAddress = rapidjson::Pointer("/data/req/virtualDeviceAddress").Get(doc)->GetInt();
        m_isSetVirtualDeviceAddress = true;
      }

      if (rapidjson::Pointer("/data/req/userData").IsValid()) {
        rapidjson::Value* userDataJson = rapidjson::Pointer("/data/req/userData").Get(doc);
        if (userDataJson->IsArray()) {
          for (rapidjson::SizeType i = 0; i < userDataJson->Size(); i++) {
            m_userData.push_back(userDataJson[i].GetInt());
          }
        }
        else {
          throw std::out_of_range("Invalid /data/req/userData");
        }
        m_isSetUserData = true;
      }
    }

    void parseVerbose(rapidjson::Document& doc) {
      if (rapidjson::Pointer("/data/returnVerbose").IsValid()) {
        m_returnVerbose = rapidjson::Pointer("/data/returnVerbose").Get(doc)->GetBool();
        m_isSetReturnVerbose = true;
      }
    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseTimeout(doc);
      parseRequest(doc);
      parseVerbose(doc);
    }
  };
}
