#pragma once

#include "IJsonSerializerService.h"
#include "ObjectFactory.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <string>

namespace iqrf {
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
