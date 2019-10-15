#pragma once

#include "IDpaTransaction2.h"
#include "IIqrfChannelService.h"
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
    
    /// Some coordinator parameters acquired during initialization
    struct CoordinatorParameters
    {
      std::string moduleId;
      std::string osVersion;
      std::string trType;
      std::string mcuType;
      std::string osBuild;
      std::string dpaVer;
      std::string dpaVerWordAsStr;
      uint16_t dpaVerWord = 0;
      uint16_t osBuildWord = 0;
      int dpaVerMajor = 0;
      int dpaVerMinor = 0;
      bool demoFlag = false;
      bool stdModeSupportFlag = false;
      bool lpModeSupportFlag = false;
      bool lpModeRunningFlag = false;
    };

    class ExclusiveAccess
    {
    public:
      virtual std::shared_ptr<IDpaTransaction2> executeDpaTransaction(const DpaMessage& request, int32_t timeout = -1) = 0;
      virtual void executeDpaTransactionRepeat( const DpaMessage & request, std::unique_ptr<IDpaTransactionResult2>& result, int repeat, int32_t timeout = -1 ) = 0;
      virtual ~ExclusiveAccess() {}
    };

    /// returns empty pointer if exclusiveAccess already assigned
    /// explicit unique_ptr::reset() or just get it out of scope of returned ptr releases exclusive access
    virtual std::unique_ptr<ExclusiveAccess> getExclusiveAccess() = 0;

    /// 0 > timeout - use default, 0 == timeout - use infinit, 0 < timeout - user value
    virtual std::shared_ptr<IDpaTransaction2> executeDpaTransaction(const DpaMessage& request, int32_t timeout = -1) = 0;
    virtual void executeDpaTransactionRepeat( const DpaMessage & request, std::unique_ptr<IDpaTransactionResult2>& result, int repeat, int32_t timeout = -1 ) = 0;
    virtual CoordinatorParameters getCoordinatorParameters() const = 0;
    virtual int getTimeout() const = 0;
    virtual void setTimeout(int timeout) = 0;
    virtual IDpaTransaction2::RfMode getRfCommunicationMode() const = 0;
    virtual void setRfCommunicationMode(IDpaTransaction2::RfMode rfMode) = 0;
    virtual IDpaTransaction2::TimingParams getTimingParams() const = 0;
    virtual void setTimingParams( IDpaTransaction2::TimingParams params ) = 0;
    virtual IDpaTransaction2::FrcResponseTime getFrcResponseTime() const = 0;
    virtual void setFrcResponseTime( IDpaTransaction2::FrcResponseTime frcResponseTime ) = 0;
    virtual void registerAsyncMessageHandler(const std::string& serviceId, AsyncMessageHandlerFunc fun) = 0;
    virtual void unregisterAsyncMessageHandler(const std::string& serviceId) = 0;
    virtual int getDpaQueueLen() const = 0;
    virtual IIqrfChannelService::State getIqrfChannelState() = 0;

    virtual ~IIqrfDpaService() {}
  };
}
