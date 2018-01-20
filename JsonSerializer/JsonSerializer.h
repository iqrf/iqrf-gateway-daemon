#pragma once

#include "IJsonSerializerService.h"
#include "ObjectFactory.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include <string>

namespace iqrf {
  class JsonSerializer : public ObjectFactory<DpaTask, rapidjson::Value>, public IJsonSerializerService
  {
  public:
    JsonSerializer();
    virtual ~JsonSerializer();

    std::string parseCategory(const std::string& request) override;
    std::unique_ptr<DpaTask> parseRequest(const std::string& request) override;
    std::string parseConfig(const std::string& request) override;
    std::string encodeConfig(const std::string& request, const std::string& response) override;
    std::string getLastError() const override;
    std::string encodeAsyncAsDpaRaw(const DpaMessage& dpaMessage) const override;

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

  private:
    std::string m_lastError;
  };

}
