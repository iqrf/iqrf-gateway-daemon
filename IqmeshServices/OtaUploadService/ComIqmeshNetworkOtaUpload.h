#pragma once

#include "ComBase.h"
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

namespace iqrf {
  class ComIqmeshNetworkOtaUpload : public ComBase
  {
  public:
    ComIqmeshNetworkOtaUpload() = delete;
    ComIqmeshNetworkOtaUpload(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkOtaUpload()
    {
    }

    int getRepeat() const {
      return m_repeat;
    }

    bool isSetDeviceAddr() const {
      return m_isSetDeviceAddr;
    }

    const std::vector<int> getDeviceAddr() const
    {
      return m_deviceAddr;
    }


    bool isSetFileName() const {
      return m_isSetFileName;
    }

    const std::string getFileName() const
    {
      return m_fileName;
    }
    

    bool isSetStartMemAddr() const {
      return m_isSetStartMemAddr;
    }

    const int getStartMemAddr() const
    {
      return m_startMemAddr;
    }


    bool isSetLoadingAction() const {
      return m_isSetLoadingAction;
    }

    const std::string getLoadingAction() const
    {
      return m_loadingAction;
    }


  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    bool m_isSetDeviceAddr = false;
    bool m_isSetFileName = false;
    bool m_isSetStartMemAddr = false;
    bool m_isSetLoadingAction = false;

    int m_repeat = 1;
    std::vector<int> m_deviceAddr;
    std::string m_fileName;
    int m_startMemAddr;
    std::string m_loadingAction;


    void parseRepeat(rapidjson::Document& doc) {
      if (rapidjson::Value* repeatJsonVal = rapidjson::Pointer("/data/repeat").Get(doc)) {
        m_repeat = repeatJsonVal->GetInt();
      }
    }

    void parseRequest(rapidjson::Document& doc) {
      if (rapidjson::Value* reqJsonVal = rapidjson::Pointer("/data/req").Get(doc)) {
        m_deviceAddr = jutils::getMemberAsVector<int>("deviceAddr", *reqJsonVal);
        m_isSetDeviceAddr = true;
      }

      if (rapidjson::Value* fileNameJsonVal = rapidjson::Pointer("/data/req/fileName").Get(doc)) {
        m_fileName = fileNameJsonVal->GetString();
        m_isSetFileName = true;
      }

      if (rapidjson::Value* startMemAddrJsonVal = rapidjson::Pointer("/data/req/startMemAddr").Get(doc)) {
        m_startMemAddr = startMemAddrJsonVal->GetInt();
        m_isSetStartMemAddr = true;
      }

      if (rapidjson::Value* loadingActionJsonVal = rapidjson::Pointer("/data/req/loadingAction").Get(doc)) {
        m_loadingAction = loadingActionJsonVal->GetString();
        m_isSetLoadingAction = true;
      }

    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRepeat(doc);
      parseRequest(doc);
    }
  };
}
