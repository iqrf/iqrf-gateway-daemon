#pragma once

#include "ShapeDefines.h"
#include <string>
#include <memory>

#ifdef IJsonSerializerService_EXPORTS
#define IJsonSerializerService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IJsonSerializerService_DECLSPEC SHAPE_ABI_IMPORT
#endif

class DpaTask;
class DpaMessage;

/// Configuration category identification string
static const std::string CAT_CONF_STR("conf");
/// DPA category identification sting
static const std::string CAT_DPA_STR("dpa");

namespace iqrf {

  class IJsonSerializerService_DECLSPEC IJsonSerializerService
  {
  public:
    virtual std::string parseCategory(const std::string& request) = 0;
    virtual std::unique_ptr<DpaTask> parseRequest(const std::string& request) = 0;
    virtual std::string parseConfig(const std::string& request) = 0;
    virtual std::string encodeConfig(const std::string& request, const std::string& response) = 0;
    virtual std::string getLastError() const = 0;
    virtual std::string encodeAsyncAsDpaRaw(const DpaMessage& dpaMessage) const = 0;
    virtual ~IJsonSerializerService() {}
  };
}
