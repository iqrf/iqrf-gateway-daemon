#pragma once

#include "IDpaTransactionResult2.h"
#include "rapidjson/pointer.h"
#include "HexStringCoversion.h"

namespace iqrf {
  class ComBase
  {
  public:
    ComBase() {}
    ComBase(const rapidjson::Document& doc)
    {
      using namespace rapidjson;
      m_mType = Pointer("/mType").Get(doc)->GetString();
      m_msgId = Pointer("/data/msgId").Get(doc)->GetString();
      const Value* timeoutVal = Pointer("/data/timeout").Get(doc);
      if (timeoutVal && timeoutVal->IsInt())
        m_timeout = timeoutVal->GetInt();
      const Value* verboseVal = Pointer("/data/returnVerbose").Get(doc);
      if (verboseVal && verboseVal->IsBool())
        m_verbose = verboseVal->GetBool();
    }

    virtual ~ComBase()
    {
    }

    const std::string& getMsgId() const { return m_msgId; }
    int32_t getTimeout() const { return m_timeout; }
    bool getVerbose() const { return m_verbose; }
    const std::string& getInsId() const { return m_insId; }
    const std::string& getStatusStr() const { return m_statusStr; }
    int getStatus() const { return m_status; }
    void setInsId(const std::string& insId) { m_insId = insId; }
    void setStatus(const std::string& statusStr, int status) {
      m_statusStr = statusStr;
      m_status = status;
    }

    const DpaMessage& getDpaRequest() const
    {
      return m_request;
    }

    void createResponse(rapidjson::Document& doc, const IDpaTransactionResult2& res)
    {
      rapidjson::Pointer("/mType").Set(doc, m_mType);
      rapidjson::Pointer("/data/msgId").Set(doc, m_msgId);
      if (m_verbose) {
        if (m_timeout != -1) {
          rapidjson::Pointer("/data/timeout").Set(doc, m_timeout);
        }
      }

      createResponsePayload(doc, res);

      if (m_verbose) {
        rapidjson::Pointer("/data/raw/0/request").Set(doc, encodeBinary(res.getRequest().DpaPacket().Buffer, res.getRequest().GetLength()));
        rapidjson::Pointer("/data/raw/0/requestTs").Set(doc, (res.getRequest().GetLength() > 0 ? encodeTimestamp(res.getRequestTs()) : ""));
        rapidjson::Pointer("/data/raw/0/confirmation").Set(doc, encodeBinary(res.getConfirmation().DpaPacket().Buffer, res.getConfirmation().GetLength()));
        rapidjson::Pointer("/data/raw/0/confirmationTs").Set(doc, (res.getConfirmation().GetLength() > 0 ? encodeTimestamp(res.getConfirmationTs()) : ""));
        rapidjson::Pointer("/data/raw/0/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
        rapidjson::Pointer("/data/raw/0/responseTs").Set(doc, (res.getResponse().GetLength() > 0 ? encodeTimestamp(res.getResponseTs()) : ""));

        rapidjson::Pointer("/data/insId").Set(doc, m_insId);
        rapidjson::Pointer("/data/statusStr").Set(doc, m_statusStr);
      }

      rapidjson::Pointer("/data/status").Set(doc, m_status);
    }

  protected:
    virtual void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) = 0;
    DpaMessage m_request;

  private:
    std::string m_mType;
    std::string m_msgId;
    int32_t m_timeout = -1;
    bool m_verbose = false;
    std::string m_insId = "iqrfgd2-1";
    std::string m_statusStr = "unknown";
    int m_status = -1;
  };
}
