/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

#include "ComBase.h"
#include "Trace.h"
#include <list>
#include "JsonUtils.h"

namespace iqrf {
  class ComIqmeshNetworkNativeUpload : public ComBase
  {
  public:
    ComIqmeshNetworkNativeUpload() = delete;
    ComIqmeshNetworkNativeUpload(rapidjson::Document& doc)
      :ComBase(doc)
    {
      parse(doc);
    }

    virtual ~ComIqmeshNetworkNativeUpload()
    {
    }

    int getRepeat() const {
      return m_repeat;
    }

    bool isSetFileName() const {
      return m_isSetFileName;
    }

    const std::string getFileName() const
    {
      return m_fileName;
    }

    bool isSetTarget() const {
      return m_isSetTarget;
    }

    const std::string getTarget() const
    {
      return m_target;
    }


  protected:
    void createResponsePayload(rapidjson::Document& doc, const IDpaTransactionResult2& res) override
    {
      rapidjson::Pointer("/data/rsp/response")
        .Set(doc, encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }


  private:
    bool m_isSetFileName = false;
    bool m_isSetTarget = false;
    
    int m_repeat = 1;
    std::string m_fileName;
    std::string m_target;
    

    void parseRepeat(rapidjson::Document& doc) {
      if (rapidjson::Value* repeatJsonVal = rapidjson::Pointer("/data/repeat").Get(doc)) {
        m_repeat = repeatJsonVal->GetInt();
      }
    }

    void parseRequest(rapidjson::Document& doc) {
      if (rapidjson::Value* fileNameJsonVal = rapidjson::Pointer("/data/req/fileName").Get(doc)) {
        m_fileName = fileNameJsonVal->GetString();
        m_isSetFileName = true;
      }

      if (rapidjson::Value* targetJsonVal = rapidjson::Pointer("/data/req/target").Get(doc)) {
        m_target = targetJsonVal->GetString();
        m_isSetTarget = true;
      }
    }

    // parses document into data fields
    void parse(rapidjson::Document& doc) {
      parseRepeat(doc);
      parseRequest(doc);
    }
  };
}
