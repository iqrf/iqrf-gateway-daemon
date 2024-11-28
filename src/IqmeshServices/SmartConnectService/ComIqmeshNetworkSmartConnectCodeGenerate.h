/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

namespace iqrf {

  // Smart Connect code generate input parameters
  typedef struct TSmartConnectCodeGenerateInputParams {
    uint8_t deviceAddress;
    int repeat;
  } TSmartConnectCodeGenerateInputParams;

  /// Smart connect code generate service com class
  class ComIqmeshNetworkSmartConnectCodeGenerate : public ComBase
  {
  public:
    /**
     * Delete default constructor
     */
    ComIqmeshNetworkSmartConnectCodeGenerate() = delete;

    /**
     * Constructor
     * @param doc Request document
     */
    explicit ComIqmeshNetworkSmartConnectCodeGenerate(rapidjson::Document& doc) : ComBase(doc) {
      parse(doc);
    }

    /**
     * Destructor
     */
    virtual ~ComIqmeshNetworkSmartConnectCodeGenerate() {}

    /**
     * Returns input parameters
     * @return Input parameters
     */
    const TSmartConnectCodeGenerateInputParams getParams() const {
      return m_params;
    }

  protected:
    /**
     * Populates response document
     * @param doc Response document
     * @param res Transaction result
     */
    void createResponsePayload(rapidjson::Document &doc, const IDpaTransactionResult2 &res) override {
      rapidjson::Pointer("/data/rsp/response").Set(doc, HexStringConversion::encodeBinary(res.getResponse().DpaPacket().Buffer, res.getResponse().GetLength()));
    }
  private:
    /**
     * Parses request document and stores DpaHops request parameters
     * @param doc Request document
     */
    void parse(rapidjson::Document &doc) {
      // Action
      rapidjson::Value *v = rapidjson::Pointer("/data/req/deviceAddr").Get(doc);
      if (v) {
        m_params.deviceAddress = static_cast<uint8_t>(v->GetUint());
      }
      // Transaction repeats
      v = rapidjson::Pointer("/data/repeat").Get(doc);
      if (v) {
        m_params.repeat = static_cast<uint8_t>(v->GetUint());
      }
    }

    /// DpaHops input parameters
    TSmartConnectCodeGenerateInputParams m_params;
  };
}
