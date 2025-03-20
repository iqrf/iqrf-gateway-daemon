/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "IChannel.h"
#include "IIqrfChannelService.h"
#include "Trace.h"

//This is workaround class just to keep clibdpa library backward compatibility to satisfy IChannel deps
//As soon the library is redesigned to Shape component it seems reasonable to pass directly IqrfChannelService

namespace iqrf {
  class IqrfDpaChannel : public IChannel
  {
  public:
    IqrfDpaChannel() = delete;
    IqrfDpaChannel(IIqrfChannelService* iqrfChannelService)
    :m_iqrfChannelService(iqrfChannelService) {}
    virtual ~IqrfDpaChannel() {}

    void sendTo(const std::basic_string<unsigned char>& message) override
    {
      if (!m_accessorExclusive) {
        m_accessor->send(message);
      }
      else {
        m_accessorExclusive->send(message);
      }
    }

    void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) override
    {
      m_receiveFromFunc = receiveFromFunc;
      m_accessor = m_iqrfChannelService->getAccess(m_receiveFromFunc, IIqrfChannelService::AccesType::Normal);
    }

    void unregisterReceiveFromHandler() override
    {
      m_accessor.reset();
      m_accessorExclusive.reset();
      m_receiveFromFunc = ReceiveFromFunc();
    }

    State getState() override
    {
      switch (m_iqrfChannelService->getState()) {
      case IIqrfChannelService::State::NotReady:
        return State::NotReady;
      case IIqrfChannelService::State::Ready:
        return State::Ready;
      default:
        return State::NotReady;
      }
      return State::NotReady;
    }

    void setExclusiveAccess()
    {
      TRC_FUNCTION_ENTER("");
      std::unique_lock<std::mutex> lck(m_accessMtx);
      m_accessorExclusive = m_iqrfChannelService->getAccess(m_receiveFromFunc, IIqrfChannelService::AccesType::Exclusive);
      TRC_FUNCTION_LEAVE("");
    }

    void resetExclusiveAccess()
    {
      TRC_FUNCTION_ENTER("");
      std::unique_lock<std::mutex> lck(m_accessMtx);
      m_accessorExclusive.reset();
      TRC_FUNCTION_LEAVE("");
    }

    bool hasOwnExclusiveAccess()
    {
      std::unique_lock<std::mutex> lck(m_accessMtx);
      return (bool)m_accessorExclusive;
    }

    bool hasExclusiveAccess()
    {
      return m_iqrfChannelService->hasExclusiveAccess();
    }

  private:
    IIqrfChannelService* m_iqrfChannelService = nullptr;
    ReceiveFromFunc m_receiveFromFunc;
    std::unique_ptr<IIqrfChannelService::Accessor> m_accessor;
    std::unique_ptr<IIqrfChannelService::Accessor> m_accessorExclusive;
    std::mutex m_accessMtx;
  };
}
