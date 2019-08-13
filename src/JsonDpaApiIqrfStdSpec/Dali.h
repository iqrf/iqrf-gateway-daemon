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
      CommandResponse() = delete;

      uint8_t getStatus() const { return m_status; }
      uint8_t getValue() const { return m_value; }

      CommandResponse(uint8_t status, uint8_t value)
        :m_status(status)
        , m_value(value)
      {}

    private:
      uint8_t m_status;
      uint8_t m_value;
    };

    ////////////////
    class Frc
    {
    protected:
      //params
      uint16_t m_daliCommand;
      std::vector<int> m_selectedNodes;

      //response
      std::vector<CommandResponse> m_items;

      Frc()
        :m_daliCommand(0)
      {}

      Frc(uint16_t daliCommand, const std::vector<int> & selectedNodes )
        :m_daliCommand(daliCommand)
        , m_selectedNodes(selectedNodes)
      {}

    public:
      virtual ~Frc() {}

      // get param data passes by ctor
      uint16_t getDaliCommand() const { return m_daliCommand; }
      const std::vector<int> & getSelectedNodes() const { return m_selectedNodes; }

      // get data as returned from driver
      const std::vector<CommandResponse> & getItems() const { return m_items; }

    };
    typedef std::unique_ptr<Frc> FrcPtr;

  } //namespace dali
} //namespace iqrf
