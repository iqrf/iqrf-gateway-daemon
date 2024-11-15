#pragma once

#include "ApiMsg.h"

namespace iqrf {

  class JsonParseErrorMsg : public ApiMsg {
  public:
    JsonParseErrorMsg() = delete;

    JsonParseErrorMsg(const std::string &message)
      : ApiMsg("error_JsonParse", "unknown", false)
    {
      setStatus("Unable to parse JSON message.", -1);
    }

    virtual ~JsonParseErrorMsg() {}

    void createResponsePayload(rapidjson::Document &doc) override {
      rapidjson::Pointer("/data/rsp/message").Set(doc, m_message);
    }
  private:
    std::string m_message;
  };

  class ApiErrorMsgBase : public ApiMsg {
  public:
    ApiErrorMsgBase() = delete;

  protected:
    ApiErrorMsgBase(const std::string &mTypeError, const std::string &msgId, const std::string &mType)
      : ApiMsg(mTypeError, msgId, true), m_mType(mType)
    {}

    virtual ~ApiErrorMsgBase() {}

    void createResponsePayload(rapidjson::Document &doc) override {
      rapidjson::Pointer("/data/rsp/mType").Set(doc, m_mType);
    }

    std::string m_mType;
  };

  class UnsupportedMsg : public ApiErrorMsgBase {
  public:
    UnsupportedMsg() = delete;

    UnsupportedMsg(const std::string &msgId, const std::string &mType)
      : ApiErrorMsgBase("error_UnsupportedMsg", msgId, mType)
    {
      setStatus("Unknown or unsupported message type.", -1); // TODO SPECIFIC ERROR CODE AND MESSAGE
    }
    
    virtual ~UnsupportedMsg() {}

    void createResponsePayload(rapidjson::Document &doc) override {
      ApiErrorMsgBase::createResponsePayload(doc);
    }
  };

  class QueueInactiveMsg : public ApiErrorMsgBase {
  public:
    QueueInactiveMsg() = delete;

    QueueInactiveMsg(const std::string &msgId, const std::string &mType, bool networkQueue = true)
      : ApiErrorMsgBase("error_MessageQueueInactive", msgId, mType), m_networkQueue(networkQueue)
    {
      setStatus("Message queue not initialized.", -1); // TODO SPECIFIC ERROR CODE AND MESSAGE
    }

    virtual ~QueueInactiveMsg() {}

    void createResponsePayload(rapidjson::Document &doc) override {
      ApiErrorMsgBase::createResponsePayload(doc);
      rapidjson::Pointer("/data/rsp/queueType").Set(doc, m_networkQueue ? "network" : "management");
    }
  private:
    bool m_networkQueue;
  };

  class QueueFullMsg : public ApiErrorMsgBase {
  public:
    QueueFullMsg() = delete;

    QueueFullMsg(const std::string &msgId, const std::string &mType, const size_t &queueLen, bool networkQueue = true)
      : ApiErrorMsgBase("error_MessageQueueFull", msgId, mType), m_queueLen(queueLen), m_networkQueue(networkQueue)
    {
      setStatus("Message queue full.", -1); // TODO SPECIFIC ERROR CODE AND MESSAGE
    }

    virtual ~QueueFullMsg() {}

    void createResponsePayload(rapidjson::Document &doc) override {
      ApiErrorMsgBase::createResponsePayload(doc);
      rapidjson::Pointer("/data/rsp/queueType").Set(doc, m_networkQueue ? "network" : "management");
      rapidjson::Pointer("/data/rsp/queueLength").Set(doc, m_queueLen);
    }
  private:
    size_t m_queueLen;
    bool m_networkQueue;
  };

  class InvalidMsg : public ApiErrorMsgBase {
  public:
    InvalidMsg() = delete;

    InvalidMsg(const std::string &msgId, const std::string &mType, const std::string &message, const std::string &violatingMember, const std::string &violation, bool request = true)
      : ApiErrorMsgBase("error_InvalidMsg", msgId, mType), m_message(message), m_member(violatingMember), m_violation(violation), m_request(request)
    {
      setStatus("Invalid message contents.", -1); // TODO SPECIFIC ERROR CODE AND MESSAGE
    }

    virtual ~InvalidMsg() {}

    void createResponsePayload(rapidjson::Document &doc) override {
      ApiErrorMsgBase::createResponsePayload(doc);
      rapidjson::Pointer("/data/rsp/direction").Set(doc, m_request ? "request" : "response");
      rapidjson::Pointer("/data/rsp/message").Set(doc, m_message);
      rapidjson::Pointer("/data/rsp/violatingMember").Set(doc, m_member);
      rapidjson::Pointer("/data/rsp/violation").Set(doc, m_violation);
    }
  private:
    /// Message containing invalid properties
    std::string m_message;
    /// Violating member
    std::string m_member;
    /// Violation description
    std::string m_violation;
    /// Message direction
    bool m_request;
  };
}
