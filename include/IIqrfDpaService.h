#pragma once

#include "ShapeDefines.h"
#include <string>
#include <functional>

#ifdef IIqrfDpaService_EXPORTS
#define IIqrfDpaService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IIqrfDpaService_DECLSPEC SHAPE_ABI_IMPORT
#endif

class DpaTransaction;
class DpaMessage;

namespace iqrfgw {
  class IIqrfDpaService_DECLSPEC IIqrfDpaService
  {
  public:
    enum RfMode {
      Std,
      Lp
    };

    /// Asynchronous DPA message handler functional type
    typedef std::function<void(const DpaMessage& dpaMessage)> AsyncMessageHandlerFunc;

    virtual void executeDpaTransaction(DpaTransaction& dpaTransaction) = 0;
    virtual void killDpaTransaction() = 0;
    virtual int getTimeout() const = 0;
    virtual void setTimeout(int timeout) = 0;
    virtual RfMode getRfCommunicationMode() const = 0;
    virtual void setRfCommunicationMode(RfMode rfMode) = 0;
    //virtual void registerAsyncMessageHandler(std::function<void(const DpaMessage&)> messageHandler) = 0;
    //virtual void unregisterAsyncMessageHandler() = 0;
    virtual void registerAsyncMessageHandler(const std::string& serviceId, AsyncMessageHandlerFunc fun) = 0;
    virtual void unregisterAsyncMessageHandler(const std::string& serviceId) = 0;

    virtual ~IIqrfDpaService() {}
  };
}
