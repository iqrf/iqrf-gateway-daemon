/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "rapidjson/pointer.h"

namespace iqrf {
  class ApiMsg
  {
  public:
    ApiMsg() {}

    ApiMsg(const std::string& mType, const std::string& msgId, bool verbose)
      :m_mType(mType)
      ,m_msgId(msgId)
      ,m_verbose(verbose)
    {
    }

    ApiMsg(const rapidjson::Document& doc)
    {
      using namespace rapidjson;
      m_mType = Pointer("/mType").Get(doc)->GetString();
      m_msgId = Pointer("/data/msgId").Get(doc)->GetString();
      const Value* verboseVal = Pointer("/data/returnVerbose").Get(doc);
      if (verboseVal && verboseVal->IsBool())
        m_verbose = verboseVal->GetBool();
    }

    virtual ~ApiMsg()
    {
    }

    const std::string& getMType() const { return m_mType; }
    const std::string& getMsgId() const { return m_msgId; }
    bool getVerbose() const { return m_verbose; }
    const std::string& getInsId() const { return m_insId; }
    const std::string& getStatusStr() const { return m_statusStr; }
    int getStatus() const { return m_status; }
    void setInsId(const std::string& insId) { m_insId = insId; }
    void setStatus(const std::string& statusStr, int status) {
      m_statusStr = statusStr;
      m_status = status;
    }

    void createResponse(rapidjson::Document& doc)
    {
      rapidjson::Pointer("/mType").Set(doc, m_mType);
      rapidjson::Pointer("/data/msgId").Set(doc, m_msgId);

      createResponsePayload(doc);

      if (m_verbose) {
        rapidjson::Pointer("/data/insId").Set(doc, m_insId);
        rapidjson::Pointer("/data/statusStr").Set(doc, m_statusStr);
      }

      rapidjson::Pointer("/data/status").Set(doc, m_status);
    }

  protected:
    virtual void createResponsePayload(rapidjson::Document& doc) = 0;
    int m_status = -1;

  private:
    std::string m_mType;
    std::string m_msgId;
    bool m_verbose = false;
    std::string m_insId = "iqrfgd2-1";
    std::string m_statusStr = "err";
  };
}
