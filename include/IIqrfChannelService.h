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
      ExclusiveAcess
    };

    enum class AccesType
    {
      Normal,
      Exclusive,
      Sniffer
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
    };

    virtual State getState() const = 0;
    virtual std::unique_ptr<Accessor> getAccess(ReceiveFromFunc receiveFromFunc, AccesType access) = 0;
    virtual ~IIqrfChannelService() {}

    class StateConvertTable
    {
    public:
      static const std::vector<std::pair<State, std::string>>& table()
      {
        static std::vector <std::pair<State, std::string>> table = {
          { State::Ready, "Ready" },
          { State::NotReady, "NotReady" },
          { State::ExclusiveAcess, "ExclusiveAcess" }
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
