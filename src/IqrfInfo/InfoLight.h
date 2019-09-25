#pragma once

#include "Light.h"

namespace iqrf
{
  namespace light
  {
    ////////////////
    class InfoEnumerate : public Enumerate
    {
    public:
      InfoEnumerate(int lightsNum)
        :Enumerate()
      {
        m_lightsNum = lightsNum;
      }

      virtual ~InfoEnumerate() {}
    };

    typedef std::unique_ptr<InfoEnumerate> InfoEnumeratePtr;

  } //namespace light
} //namespace iqrf
