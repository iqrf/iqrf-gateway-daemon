#pragma once

#include <string>
#include <set>
#include <vector>
#include <memory>

namespace iqrf
{
  namespace dali
  {
    class CommandResponse
    {
    public:
      CommandResponse()
      {}
      uint8_t getStatus() const { return m_status; }
      uint8_t getValue() const { return m_value; }
    private:
      uint8_t m_status;
      uint8_t m_value;
    };

    ////////////////
    class Frc
    {
    protected:
      //params
      uint16_t m_command;
      std::vector<int> m_selectedNodes;

      //response
      //todo

      Frc()
        :m_command(0)
      {}

      Frc(int command, const std::vector<int> & selectedNodes )
        :m_command(command)
        , m_selectedNodes(selectedNodes)
      {}

    public:
      virtual ~Frc() {}


    };
    typedef std::unique_ptr<Frc> FrcPtr;

  } //namespace sensor
} //namespace iqrf
