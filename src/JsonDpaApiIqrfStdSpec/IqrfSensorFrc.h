#pragma once

#include "ApiMsgIqrfStandard.h"

namespace iqrf {
  class ApiMsgIqrfStandardFrc : public ApiMsgIqrfStandard
  {
  public:
    ApiMsgIqrfStandardFrc() = delete;
    ApiMsgIqrfStandardFrc(const rapidjson::Document& doc)
      :ApiMsgIqrfStandard(doc)
    {
      using namespace rapidjson;
      
      const Value *selectedNodesArr = Pointer("/data/req/param/selectedNodes").Get(doc); //optional
      if (selectedNodesArr) {
        for (const Value *nodeVal = selectedNodesArr->Begin(); nodeVal != selectedNodesArr->End(); nodeVal++) {
          m_selectedNodes.push_back(nodeVal->GetInt());
        }
      }

      const Value *valGetEx = Pointer("/data/req/param/getExtraResult").Get(doc);
      if (valGetEx && valGetEx->IsBool()) {
        m_getExtraResult = valGetEx->GetBool();
      }
    }

    const std::vector<int> & getSelectedNodes() const { return m_selectedNodes; }
    bool getExtraResult() const { return m_getExtraResult; }
    
    //const DpaMessage& getDpaRequestExtra() const { return m_dpaRequestExtra; }
    //void setDpaRequestExtra(const DpaMessage& dpaRequest) { m_dpaRequestExtra = dpaRequest; }

    //const DpaMessage* getDpaResponse() const
    //{
    //  if (m_res) return &m_res->getResponse();
    //  return nullptr;
    //}

    //const DpaMessage* getDpaResponseExtra() const
    //{
    //  if (m_extraRes) return &m_extraRes->getResponse();
    //  return nullptr;
    //}

    void setDpaTransactionExtraResult(std::unique_ptr<IDpaTransactionResult2> extraRes)
    {
      m_extraRes = std::move(extraRes);
    }

    virtual ~ApiMsgIqrfStandardFrc()
    {
    }

  protected:
    void createResponsePayload(rapidjson::Document& doc) override
    {
      using namespace rapidjson;
      ApiMsgIqrfStandard::createResponsePayload(doc);

      bool r = (bool)m_extraRes;
      if (getVerbose() && r) {
        rapidjson::Pointer("/data/raw/1/request").Set(doc, r ? encodeBinary(m_extraRes->getRequest().DpaPacket().Buffer, m_extraRes->getRequest().GetLength()) : "");
        rapidjson::Pointer("/data/raw/1/requestTs").Set(doc, r ? encodeTimestamp(m_extraRes->getRequestTs()) : "");
        rapidjson::Pointer("/data/raw/1/confirmation").Set(doc, r ? encodeBinary(m_extraRes->getConfirmation().DpaPacket().Buffer, m_extraRes->getConfirmation().GetLength()) : "");
        rapidjson::Pointer("/data/raw/1/confirmationTs").Set(doc, r ? encodeTimestamp(m_extraRes->getConfirmationTs()) : "");
        rapidjson::Pointer("/data/raw/1/response").Set(doc, r ? encodeBinary(m_extraRes->getResponse().DpaPacket().Buffer, m_extraRes->getResponse().GetLength()) : "");
        rapidjson::Pointer("/data/raw/1/responseTs").Set(doc, r ? encodeTimestamp(m_extraRes->getResponseTs()) : "");
      }
    }

  private:
    bool m_getExtraResult = true;
    std::vector<int> m_selectedNodes;
    DpaMessage m_dpaRequestExtra;
    std::unique_ptr<IDpaTransactionResult2> m_extraRes;
  };

  class IqrfSensorFrc : public ApiMsgIqrfStandardFrc
  {
  public:
    IqrfSensorFrc() = delete;
    IqrfSensorFrc(const rapidjson::Document& doc)
      :ApiMsgIqrfStandardFrc(doc)
    {
      using namespace rapidjson;
      m_sensorType = Pointer("/data/req/param/sensorType").Get(doc)->GetInt();
      m_frcCommand = Pointer("/data/req/param/frcCommand").Get(doc)->GetInt();
    }

    int getSensorType() const { return m_sensorType; }
    int getFrcCommand() const { return m_frcCommand; }
    
    virtual ~IqrfSensorFrc()
    {}

  protected:
    void createResponsePayload(rapidjson::Document& doc) override
    {
      using namespace rapidjson;
      ApiMsgIqrfStandardFrc::createResponsePayload(doc);
    }

  private:
    int m_sensorType = 0;
    int m_frcCommand = 0;
  };

  class IqrfDaliFrc : public ApiMsgIqrfStandardFrc
  {
  public:
    IqrfDaliFrc() = delete;
    IqrfDaliFrc(const rapidjson::Document& doc)
      :ApiMsgIqrfStandardFrc(doc)
    {
      using namespace rapidjson;
      m_daliCommand = Pointer("/data/req/param/command").Get(doc)->GetInt();
    }

    int getCommand() const { return m_daliCommand; }

    virtual ~IqrfDaliFrc()
    {}

  protected:
    void createResponsePayload(rapidjson::Document& doc) override
    {
      using namespace rapidjson;
      ApiMsgIqrfStandardFrc::createResponsePayload(doc);
    }

  private:
    int m_daliCommand = 0;
  };

}
