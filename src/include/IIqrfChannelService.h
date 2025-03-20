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

#include "ShapeDefines.h"
#include "EnumStringConvertor.h"
#include <string>
#include <functional>
#include <memory>

#ifdef IIqrfChannelService_EXPORTS
#define IIqrfChannelService_DECLSPEC SHAPE_ABI_EXPORT
#else
#define IIqrfChannelService_DECLSPEC SHAPE_ABI_IMPORT
#endif

namespace iqrf {
  class IIqrfChannelService_DECLSPEC IIqrfChannelService
  {
  public:
    enum class State
    {
      Ready,
      NotReady,
      ExclusiveAccess
    };

    enum class AccesType
    {
      Normal,
      Exclusive,
      Sniffer
    };

    // target of upload
    enum class UploadTarget
    {
      UPLOAD_TARGET_CFG,
      UPLOAD_TARGET_RFPMG,
      UPLOAD_TARGET_RFBAND,
      UPLOAD_TARGET_ACCESS_PWD,
      UPLOAD_TARGET_USER_KEY,
      UPLOAD_TARGET_FLASH,
      UPLOAD_TARGET_INTERNAL_EEPROM,
      UPLOAD_TARGET_EXTERNAL_EEPROM,
      UPLOAD_TARGET_SPECIAL
    };

    // error codes of upload operation
    enum class UploadErrorCode {
      UPLOAD_NO_ERROR = 0,
      UPLOAD_ERROR_GENERAL,
      UPLOAD_ERROR_TARGET_MEMORY,
      UPLOAD_ERROR_DATA_LEN,
      UPLOAD_ERROR_ADDRESS,
      UPLOAD_ERROR_WRITE_ONLY,
      UPLOAD_ERROR_COMMUNICATION,
      UPLOAD_ERROR_NOT_SUPPORTED,
      UPLOAD_ERROR_BUSY
    };

    struct osInfo {
      uint16_t osBuild;
      uint8_t osVersionMajor, osVersionMinor;
    };

    // receive data handler
    typedef std::function<int(const std::basic_string<unsigned char>&)> ReceiveFromFunc;

    class Accessor
    {
    public:
      //TODO return value if message was not send because of access refuse
      virtual void send(const std::basic_string<unsigned char>& message) = 0;

      virtual AccesType getAccessType() = 0;
      virtual ~Accessor() {}

      virtual bool enterProgrammingState() = 0;
      virtual bool terminateProgrammingState() = 0;

      // 'address' parameter is NOT used, if upload target has already defined its own address,
      // which to upload code into, e.g. RF band or RFPGM
      virtual UploadErrorCode upload(
        const UploadTarget target,
        const std::basic_string<uint8_t>& data,
        const uint16_t address
      ) = 0;

      virtual IIqrfChannelService::osInfo getTrModuleInfo() = 0;
    };

    virtual void startListen() = 0;
    virtual State getState() const = 0;
    virtual std::unique_ptr<Accessor> getAccess(ReceiveFromFunc receiveFromFunc, AccesType access) = 0;
    virtual bool hasExclusiveAccess() const = 0;

    virtual ~IIqrfChannelService() {}

    class StateConvertTable
    {
    public:
      static const std::vector<std::pair<State, std::string>>& table()
      {
        static std::vector <std::pair<State, std::string>> table = {
          { State::Ready, "Ready" },
          { State::NotReady, "NotReady" },
          { State::ExclusiveAccess, "ExclusiveAccess" }
        };
        return table;
      }
      static State defaultEnum()
      {
        return State::NotReady;
      }
      static const std::string& defaultStr()
      {
        static std::string u("unknown");
        return u;
      }
    };

    typedef shape::EnumStringConvertor<State, StateConvertTable> StateStringConvertor;
  };
}
