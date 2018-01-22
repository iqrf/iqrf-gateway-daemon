#pragma once

#include "ShapeDefines.h"
#include <string>
#include <functional>

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
      NotReady
    };

    // receive data handler
    typedef std::function<int(const std::basic_string<unsigned char>&)> ReceiveFromFunc;

    virtual void sendTo(const std::basic_string<unsigned char>& message) = 0;
    virtual void registerReceiveFromHandler(ReceiveFromFunc receiveFromFunc) = 0;
    virtual void unregisterReceiveFromHandler() = 0;
    virtual State getState() const = 0;
    virtual ~IIqrfChannelService() {}
  };
}
