#pragma once

#include <string>
#include <set>
#include <vector>
#include <memory>

namespace iqrf
{
  namespace dali
  {
    namespace item
    {
      class Answer
      {
      public:
        Answer(uint8_t status, uint8_t value)
          :m_status(status)
          , m_value(value)
        {}

        uint8_t getStatus() const { return m_status; }
        uint8_t getValue() const { return m_value; }

      protected:
        Answer()
        {}

        uint8_t m_status = 0;
        uint8_t m_value = 0;
      };
      typedef std::unique_ptr<Answer> AnswerPtr;
    }

    ////////////////
    // not implemented as DPA command just kept here for future use and to be consistent with other standards
    class Enumerate
    {
    protected:
      Enumerate()
      {}

    public:
      virtual ~Enumerate() {}
    };
    typedef std::unique_ptr<Enumerate> EnumeratePtr;

    ////////////////
    class SendCommands
    {
    protected:
      //param
      std::vector<uint16_t> m_commands;
      //response
      std::vector<item::AnswerPtr> m_answers;

      SendCommands()
      {}

      SendCommands(const std::vector<uint16_t> & commands)
      {
        (void)commands; //silence -Wunused-parameter
      }

    public:
      // get param data passes by ctor
      const std::vector<uint16_t> & getCommands() const { return m_commands; }

      // get data as returned from driver
      const std::vector<item::AnswerPtr> & getAnswers() const { return m_answers; }

      virtual ~SendCommands() {}
    };
    typedef std::unique_ptr<SendCommands> SendCommandsPtr;

    ////////////////
    class SendCommandsAsync : public SendCommands
    {
    protected:
      SendCommandsAsync()
        :SendCommands()
      {}

      SendCommandsAsync(const std::vector<uint16_t> & commands)
        :SendCommands(commands)
      {}

    public:
      virtual ~SendCommandsAsync() {}
    };
    typedef std::unique_ptr<SendCommands> SendCommandsPtr;

    ////////////////
    class Frc
    {
    protected:
      //params
      uint16_t m_daliCommand;
      std::vector<int> m_selectedNodes;

      //response
      std::vector<item::AnswerPtr> m_answers;

      Frc()
        :m_daliCommand(0)
      {}

      Frc(uint16_t daliCommand)
        :m_daliCommand(daliCommand)
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
      // answers may be nullptr if node doesn't return valid value
      const std::vector<item::AnswerPtr> & getAnswersPerNode() const { return m_answers; }

    };
    typedef std::unique_ptr<Frc> FrcPtr;

  } //namespace dali
} //namespace iqrf
