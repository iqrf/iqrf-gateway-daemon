#pragma once

#include <string>

namespace iqrf
{
  namespace binaryoutput
  {
    ////////////////
    class Enumerate
    {
    protected:
      int m_outputsNum;

      Enumerate()
      {}

    public:
      virtual ~Enumerate() {}

      int getBinaryOutputsNum() const { return m_outputsNum; }
    };
    typedef std::unique_ptr<Enumerate> EnumeratePtr;

  } //namespace binaryoutput
} //namespace iqrf
