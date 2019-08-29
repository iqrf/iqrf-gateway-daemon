#pragma once

#include "Dali.h"

namespace iqrf
{
  namespace dali
  {
    ////////////////
    class InfoEnumerate : public Enumerate
    {
    public:
      InfoEnumerate()
        :Enumerate()
      {}
      virtual ~InfoEnumerate() {}
    };
    typedef std::unique_ptr<InfoEnumerate> InfoEnumeratePtr;

  } //namespace dali
} //namespace iqrf
