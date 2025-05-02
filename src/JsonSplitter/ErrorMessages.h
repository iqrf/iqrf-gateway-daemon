/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
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

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/pointer.h"

/**
 * Base messageError class
 */
class BaseErrorMsg {
protected:

    /**
     * Message error status codes
     */
    enum ErrorMsgCodes {
        General = 1,
        JsonParse,
        MissingMessageType,
        Validation,
        ManagementQueueInactive,
        ManagementQueueFull,
        NetworkQueueInactive,
        NetworkQueueFull,
    };

    /**
     * Create base rapidjson document containing shared properties of messageError API messages
     * @param msgId Message ID
     * @return Base messageError document
     */
    static rapidjson::Document createMessage(const std::string &msgId) {
        rapidjson::Document doc(rapidjson::kObjectType);
        rapidjson::Pointer("/mType").Set(doc, "messageError");
        rapidjson::Pointer("/data/msgId").Set(doc, msgId);
        return doc;
    }
};

/**
 * General messageError class
 */
class GeneralErrorMsg : protected BaseErrorMsg {
public:

    /**
     * Populate general error message
     * @param msgId Message ID
     * @param msg Received message
     * @param error Error
     * @return General messageError document
     */
    static rapidjson::Document createMessage(const std::string &msgId, const std::string &msg, const std::string &error) {
        auto doc = BaseErrorMsg::createMessage(msgId);
        rapidjson::Pointer("/data/rsp/message").Set(doc, msg);
        rapidjson::Pointer("/data/rsp/error").Set(doc, error);
        rapidjson::Pointer("/data/status").Set(doc, ErrorMsgCodes::General);
        rapidjson::Pointer("/data/statusStr").Set(doc, "An error has occurred while handling request.");
        return doc;
    }
};

/**
 * JSON parse messageError class
 */
class JsonParseErrorMsg : protected BaseErrorMsg {
public:

    /**
     * Populate JSON parse error message
     * @param msg Received message
     * @param code Parsing error code
     * @param offset Position of error in JSON string
     * @return JSON parse messageError document
     */
    static rapidjson::Document createMessage(const std::string &msg, const rapidjson::ParseErrorCode &code, size_t offset) {
        auto doc = BaseErrorMsg::createMessage("unknown");
        rapidjson::Pointer("/data/rsp/message").Set(doc, msg);
        rapidjson::Pointer("/data/rsp/error").Set(doc, rapidjson::GetParseError_En(code));
        rapidjson::Pointer("/data/rsp/offset").Set(doc, offset);
        rapidjson::Pointer("/data/status").Set(doc, ErrorMsgCodes::JsonParse);
        rapidjson::Pointer("/data/statusStr").Set(doc, "Failed to parse JSON message.");
        return doc;
    }
};

/**
 * Missing message type messageError class
 */
class MissingMTypeMsg : protected BaseErrorMsg {
public:

    /**
     * Populate missing message type error message
     * @param msgId Message ID
     * @param mmsg Received message
     * @return Missing mType messageError document
     */
    static rapidjson::Document createMessage(const std::string &msgId, const std::string &msg) {
        auto doc = BaseErrorMsg::createMessage(msgId);
        rapidjson::Pointer("/data/rsp/message").Set(doc, msg);
        rapidjson::Pointer("/data/status").Set(doc, ErrorMsgCodes::MissingMessageType);
        rapidjson::Pointer("/data/statusStr").Set(doc, "mType missing in JSON message.");
        return doc;
    }
};

/**
 * Validation failure messageError class
 */
class ValidationErrorMsg : protected BaseErrorMsg {
public:

    /**
     * Populate validation failure error message
     * @param msgId Message ID
     * @param msg Received message
     * @param error Validation error
     * @return Validation failure messageError document
     */
    static rapidjson::Document createMessage(const std::string &msgId, const std::string &msg, const std::string &error) {
        auto doc = BaseErrorMsg::createMessage(msgId);
        rapidjson::Pointer("/data/rsp/message").Set(doc, msg);
        rapidjson::Pointer("/data/rsp/error").Set(doc, error);
        rapidjson::Pointer("/data/status").Set(doc, ErrorMsgCodes::Validation);
        rapidjson::Pointer("/data/statusStr").Set(doc, "Failed to validate JSON message contents.");
        return doc;
    }
};

/**
 * Message queue not initialized messageError class
 */
class MessageQueueNotInitializedErrorMsg : protected BaseErrorMsg {
public:

    /**
     * Populate uninitialized message queue error message
     * @param msgId Message ID
     * @param mType Ignored message type
     * @param networkQueue Network queue if true, otherwise management queue
     * @return Message queue not initialized messageError document
     */
    static rapidjson::Document createMessage(const std::string &msgId, const std::string &mType, bool networkQueue) {
        auto doc = BaseErrorMsg::createMessage(msgId);
        rapidjson::Pointer("/data/rsp/ignoredMessage").Set(doc, mType);
        rapidjson::Pointer("/data/status").Set(
            doc,
            networkQueue ? ErrorMsgCodes::NetworkQueueInactive : ErrorMsgCodes::ManagementQueueInactive
        );
        rapidjson::Pointer("/data/statusStr").Set(
            doc,
            networkQueue ? "Network queue is not initialized." : "Management queue is not initialized."
        );
        return doc;
    }
};

/**
 * Message queue full messageError class
 */
class MessageQueueFullErrorMsg : protected BaseErrorMsg {
public:

    /**
     * Populate full message queue error message
     * @param msgId Message ID
     * @param mType Ignored message type
     * @param networkQueue Network queue if true, otherwise management queue
     * @param capacity Queue capacity
     * @return Message queue full messageError document
     */
    static rapidjson::Document createMessage(const std::string &msgId, const std::string &mType, bool networkQueue, size_t capacity) {
        auto doc = BaseErrorMsg::createMessage(msgId);
        rapidjson::Pointer("/data/rsp/ignoredMessage").Set(doc, mType);
        rapidjson::Pointer("/data/rsp/capacity").Set(doc, capacity);
        rapidjson::Pointer("/data/status").Set(
            doc,
            networkQueue ? ErrorMsgCodes::NetworkQueueFull : ErrorMsgCodes::ManagementQueueFull
        );
        rapidjson::Pointer("/data/statusStr").Set(
            doc,
            networkQueue ? "Network queue is full." : "Management queue is full."
        );
        return doc;
    }
};
