#pragma once

#include "DpaHandler2.h"
#include "rapidjson/pointer.h"
#include "HexStringCoversion.h"

namespace iqrf {
  class ComBase
  {
  public:
    ComBase() {}
    ComBase(rapidjson::Document& doc)
    {
      m_msgId = rapidjson::Pointer("/data/msgId").Get(doc)->GetString();
      m_timeout = rapidjson::Pointer("/data/timeout").GetWithDefault(doc, (int)m_timeout).GetInt();
      m_verbose = rapidjson::Pointer("/data/returnVerbose").GetWithDefault(doc, m_verbose).GetBool();
    }

    virtual ~ComBase()
    {

    }

    const std::string& getMsgId() const
    {
      return m_msgId;
    }

    const int32_t getTimeout() const
    {
      return m_timeout;
    }

    const bool getVerbose() const
    {
      return m_verbose;
    }

    const DpaMessage& getDpaRequest() const
    {
      return m_request;
    }

    void createResponse(rapidjson::Document& doc, const IDpaTransactionResult2& res)
    {
      rapidjson::Pointer("/mType").Set(doc, "unknown-mType"); // it's here just to keep order - replaced later in processing, 
      rapidjson::Pointer("/data/msgId").Set(doc, m_msgId);
      if (m_verbose) {
        if (m_timeout != -1) {
          rapidjson::Pointer("/data/timeout").Set(doc, m_timeout);
        }
      }

      createResponsePayload(doc, res);

      if (m_verbose) {
        rapidjson::Pointer("/data/raw/request").Set(doc, encodeBinary(res.getRequest().DpaPacket().Buffer, res.getRequest().GetLength()));
        rapidjson::Pointer("/data/raw/requestTs").Set(doc, encodeTimestamp(res.getRequestTs()));
        rapidjson::Pointer("/data/raw/confirmation").Set(doc, encodeBinary(res.getConfirmation().DpaPacket().Buffer, res.getConfirmation().GetLength()));
        rapidjson::Pointer("/data/raw/confirmationTs").Set(doc, encodeTimestamp(res.getConfirmationTs()));
        rapidjson::Pointer("/data/raw/response").Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
        rapidjson::Pointer("/data/raw/responseTs").Set(doc, encodeTimestamp(res.getResponseTs()));

        rapidjson::Pointer("/data/insId").Set(doc, "iqrfgd2-1"); // TODO replace by daemon instance id
        rapidjson::Pointer("/data/statusStr").Set(doc, res.getErrorString());
      }

      rapidjson::Pointer("/data/status").Set(doc, res.getErrorCode());
    }

  protected:
    virtual void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) = 0;
    DpaMessage m_request;

  private:
    std::string m_msgId;
    int32_t m_timeout = -1;
    bool m_verbose = false;
  };
}
