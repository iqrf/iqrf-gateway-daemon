#pragma once

#include "BinaryOutput.h"

namespace iqrf
{
  namespace binaryoutput
  {
    ////////////////
    class InfoEnumerate : public Enumerate
    {
    public:
      InfoEnumerate(int outputsNum)
        :Enumerate()
      {
        m_outputsNum = outputsNum;
      }

      virtual ~InfoEnumerate() {}
    };

    typedef std::unique_ptr<InfoEnumerate> InfoEnumeratePtr;

  } //namespace binaryoutput
} //namespace iqrf
