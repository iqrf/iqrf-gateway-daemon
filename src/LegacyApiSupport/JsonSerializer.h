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

#include "DpaTask.h"
#include "ObjectFactory.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <string>

namespace iqrf {
  /// Configuration category identification string
  static const std::string CAT_CONF_STR("conf");
  /// DPA category identification sting
  static const std::string CAT_DPA_STR("dpa");

  class JsonSerializer : public ObjectFactory<DpaTask, rapidjson::Value>
  {
  public:
    JsonSerializer();
    virtual ~JsonSerializer();

    std::string parseCategory(const std::string& request);
    std::unique_ptr<DpaTask> parseRequest(const std::string& request);
    std::string parseConfig(const std::string& request);
    std::string encodeConfig(const std::string& request, const std::string& response);
    std::string getLastError() const;
    std::string encodeAsyncAsDpaRaw(const DpaMessage& dpaMessage) const;

  private:
    std::string m_lastError;
  };

}
