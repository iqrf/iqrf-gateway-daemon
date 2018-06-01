#pragma once

#include "DpaHandler2.h" //TODO just because of 
#include "ShapeDefines.h"
#include <string>
#include <functional>

#ifdef IIqrfDpaService_EXPORTS
#define IIqrfDpaService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IIqrfDpaService_DECLSPEC SHAPE_ABI_IMPORT
#endif

class DpaMessage;

namespace iqrf {
  class IIqrfDpaService_DECLSPEC IIqrfDpaService
  {
  public:
    /// Asynchronous DPA message handler functional type
    typedef std::function<void(const DpaMessage& dpaMessage)> AsyncMessageHandlerFunc;

    /// 0 > timeout - use default, 0 == timeout - use infinit, 0 < timeout - user value
    virtual std::shared_ptr<IDpaTransaction2> executeDpaTransaction(const DpaMessage& request, int32_t timeout = -1) = 0;
    virtual int getTimeout() const = 0;
    virtual void setTimeout(int timeout) = 0;
    virtual IDpaTransaction2::RfMode getRfCommunicationMode() const = 0;
    virtual void setRfCommunicationMode(IDpaTransaction2::RfMode rfMode) = 0;
    virtual void registerAsyncMessageHandler(const std::string& serviceId, AsyncMessageHandlerFunc fun) = 0;
    virtual void unregisterAsyncMessageHandler(const std::string& serviceId) = 0;

    virtual ~IIqrfDpaService() {}
  };
}
