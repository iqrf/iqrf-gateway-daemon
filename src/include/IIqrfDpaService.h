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
    typedef std::function<void(const DpaMessage& dpaMessage)> AnyMessageHandlerFunc;
    typedef std::function<void(uint8_t pnum, uint8_t pcmd)> DpaCommandHookHandlerFunc;

    enum class DpaState
    {
      Ready,
      NotReady
    };

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
      uint16_t hwpid = 0;
      uint16_t hwpidVersion = 0;
      uint8_t osVersionByte = 0;
      uint8_t trMcuType = 0;
      uint32_t mid = 0;
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
    typedef std::unique_ptr<IIqrfDpaService::ExclusiveAccess> ExclusiveAccessPtr;

    class DpaStateConvertTable
    {
    public:
      static const std::vector<std::pair<DpaState, std::string>>& table()
      {
        static std::vector <std::pair<DpaState, std::string>> table = {
          { DpaState::Ready, "Ready" },
          { DpaState::NotReady, "NotReady" }
        };

        return table;
      }

      static DpaState defaultEnum()
      {
        return DpaState::NotReady;
      }

      static const std::string& defaultStr()
      {
        static std::string u("unknown");
        return u;
      }
    };
    typedef shape::EnumStringConvertor<DpaState, DpaStateConvertTable> DpaStateStringConvertor;

    /// returns empty pointer if exclusiveAccess already assigned
    /// explicit unique_ptr::reset() or just get it out of scope of returned ptr releases exclusive access
    virtual ExclusiveAccessPtr getExclusiveAccess() = 0;
    virtual bool hasExclusiveAccess() const = 0;

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
    virtual DpaState getDpaChannelState() = 0;
    virtual void registerAnyMessageHandler(const std::string& serviceId, AnyMessageHandlerFunc fun) = 0;
    virtual void unregisterAnyMessageHandler(const std::string& serviceId) = 0;
    virtual void reinitializeCoordinator() = 0;

    virtual ~IIqrfDpaService() {}
  };
}
