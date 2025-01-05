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

#include "EmbedNode.h"
#include "IDpaTransactionResult2.h"
#include "JsDriverSolver.h"
#include "JsonUtils.h"
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace iqrf
{
  // use as common functionality predecessor for JsDriverDali, JsDriverSensor
  class JsDriverStandardFrcSolver : public JsDriverSolver
  {
  private:
    std::string m_functionName;
    DpaMessage m_frcRequest;
    uint16_t m_hwpid;
    DpaMessage m_frcExtraRequest;
    DpaMessage m_frcResponse;
    DpaMessage m_frcExtraResponse;
    std::unique_ptr<IDpaTransactionResult2> m_frcDpaTransactionResult;
    std::unique_ptr<IDpaTransactionResult2> m_frcExtraDpaTransactionResult;
    rapidjson::Document m_frcRequestResult0Doc;
    rapidjson::Document m_frcResponseResultDoc;

  protected:
    // used by sensor::JsDriverFrc, dali::JsDriverFrc
    JsDriverStandardFrcSolver(IJsRenderService* iJsRenderService)
      :JsDriverSolver(iJsRenderService)
      , m_hwpid(0xFFFF)
    {}

  public:
    JsDriverStandardFrcSolver(IJsRenderService* iJsRenderService, const std::string & functionName, const rapidjson::Value & val, uint16_t hwpid)
      :JsDriverSolver(iJsRenderService)
      , m_functionName(functionName)
      , m_hwpid(hwpid)
    {
      setRequestParamDoc(val);
    }

    virtual ~JsDriverStandardFrcSolver() {}

    const DpaMessage & getFrcRequest() const { return m_frcRequest; }
    const DpaMessage & getFrcExtraRequest() const { return m_frcExtraRequest; }

    const rapidjson::Document & getRequestResult0Doc() const { return m_frcRequestResult0Doc; }
    const rapidjson::Document & getResponseResultDoc() const { return m_frcResponseResultDoc; }

    void setFrcDpaTransactionResult(std::unique_ptr<IDpaTransactionResult2> res)
    {
      m_frcDpaTransactionResult = std::move(res);

      if (!m_frcDpaTransactionResult->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No Frc response");
      }

      m_frcResponse = m_frcDpaTransactionResult->getResponse();
    }

    void setFrcExtraDpaTransactionResult(std::unique_ptr<IDpaTransactionResult2> res)
    {
      m_frcExtraDpaTransactionResult = std::move(res);

      if (!m_frcExtraDpaTransactionResult->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No Frc Extra response");
      }

      m_frcExtraResponse = m_frcExtraDpaTransactionResult->getResponse();
    }

    std::unique_ptr<IDpaTransactionResult2> moveFrcDpaTransactionResult()
    {
      return std::move(m_frcDpaTransactionResult);
    }

    std::unique_ptr<IDpaTransactionResult2> moveFrcExtraDpaTransactionResult()
    {
      return std::move(m_frcExtraDpaTransactionResult);
    }

    const rapidjson::Document & getResponseResultDoc() { return m_responseResultDoc; }

    void filterSelectedNodes(json &doc, const std::string &path, size_t size) {
      json itemArray = doc[path];

      if (!itemArray.is_array()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Expected member " << path << " to be an array.");
      }

      if (itemArray.size() == 0) {
        return;
      }

      if (size + 1 > itemArray.size()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Result member " << path << " contains less items than requested nodes.");
      }

      doc[path] = json(itemArray.begin() + 1, itemArray.begin() + 1 + size);
    }

    void convertToExtendedFormat(json &doc, const std::string &arrayPath, const std::string &itemPath, const std::set<uint8_t> &selected, std::map<uint8_t, embed::node::NodeMidHwpid> &nodeMap) {
      json itemArray = doc[arrayPath];

      if (!itemArray.is_array()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Expected member " << arrayPath << " to be an array.");
      }

      if (itemArray.size() == 0) {
        return;
      }

      std::set<uint8_t> addrs;
      if (selected.empty()) {
        auto itr = addrs.begin();
        for (uint8_t i = 0; i < itemArray.size(); ++i) {
          itr = addrs.insert(itr, i);
        }
      } else {
        addrs = selected;
      }

      if (addrs.size() != itemArray.size()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Addr count does not match result item count.");
      }

      auto itr = addrs.begin();
      json convertedArray = json::array();
      for (auto arrayItr = itemArray.begin(); arrayItr != itemArray.end(); ++arrayItr) {
        auto addr = *itr++;
        json convertedItem = {
          {"nAdr", addr},
          {itemPath, *arrayItr}
        };
        if (nodeMap.count(addr)) {
          auto nodeItem = nodeMap.at(addr);
          convertedItem["mid"] = nodeItem.getMid();
          convertedItem["hwpid"] = nodeItem.getHwpid();
        } else {
          convertedItem["mid"] = nullptr;
          convertedItem["hwpid"] = nullptr;
        }
        
        convertedArray.push_back(convertedItem);
      }
      doc[arrayPath] = convertedArray;
    }

  protected:

    uint16_t getNadrDrv() const override
    {
      return (uint16_t)0; // coordinator
    }

    uint16_t getHwpidDrv() const
    {
      return m_hwpid;
    }

    std::string functionName() const override
    {
      return m_functionName;
    }

    void preRequest(rapidjson::Document& requestResultDoc) override
    {
      // set in ctor by setRequestParamDoc(val);
      (void)requestResultDoc; //silence -Wunused-parameter
    }

    void postRequest(const rapidjson::Document& requestResultDoc) override
    {
      using namespace rapidjson;

      if (const Value *val0 = Pointer("/retpars/0").Get(requestResultDoc)) {
        uint8_t pnum, pcmd;
        rawHdp2dpaRequest(m_frcRequest, getNadrDrv(), pnum, pcmd, getHwpidDrv(), *val0);
        m_frcRequestResult0Doc.CopyFrom(*val0, m_frcRequestResult0Doc.GetAllocator());
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array .../retpars[0]");
      }

      if (const Value *val1 = Pointer("/retpars/1").Get(requestResultDoc)) {
        uint8_t pnum, pcmd;
        rawHdp2dpaRequest(m_frcExtraRequest, getNadrDrv(), pnum, pcmd, getHwpidDrv(), *val1);
      }
      else {
        THROW_EXC_TRC_WAR(std::logic_error, "Expected: Json Array .../retpars[1]");
      }
    }

    void preResponse(rapidjson::Document& responseParamDoc) override
    {
      using namespace rapidjson;

      // some std FRC needs requestParam to parse response
      responseParamDoc.CopyFrom(getRequestParamDoc(), responseParamDoc.GetAllocator());

      if (!m_frcDpaTransactionResult->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No Frc response");
      }
      {
        Value val;
        dpa2rawHdpResponse(m_frcResponse, val, responseParamDoc.GetAllocator());
        Pointer("/responseFrcSend").Set(responseParamDoc, val);
      }

      if (m_frcExtraDpaTransactionResult) {
        // optional extra result
        if (!m_frcExtraDpaTransactionResult->isResponded()) {
          THROW_EXC_TRC_WAR(std::logic_error, "No Frc response");
        }
        {
          Value val;
          dpa2rawHdpResponse(m_frcExtraResponse, val, responseParamDoc.GetAllocator());
          Pointer("/responseFrcExtraResult").Set(responseParamDoc, val);
        }
      }

      Pointer("/frcSendRequest").Set(responseParamDoc, m_frcRequestResult0Doc);
    }

    void postResponse(const rapidjson::Document& responseResultDoc) override
    {
      m_frcResponseResultDoc.CopyFrom(responseResultDoc, m_frcResponseResultDoc.GetAllocator());
    }

  };

} //namespace iqrf
