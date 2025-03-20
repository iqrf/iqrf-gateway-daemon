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

#include <string>
#include <set>
#include <vector>
#include <memory>

namespace iqrf {
  namespace light {
    namespace item {
      class Answer {
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
    class SendLdiCommands {
    protected:
      /**
       * Base constructor
       */
      SendLdiCommands() {}

      /**
       * Commands constructor
       */
      SendLdiCommands(const std::vector<uint16_t> &ldiCommands): m_ldiCommands(ldiCommands) {}

      /// LDI commands
      std::vector<uint16_t> m_ldiCommands;
      /// LDI answers
      std::vector<item::AnswerPtr> m_answers;
    public:
      /**
       * Destructor
       */
      virtual ~SendLdiCommands() {}

      /**
       * Get LDI commands
       * @return std::vector<uint16_t> LDI commands
       */
      const std::vector<uint16_t>& getLdiCommands() const { return m_ldiCommands; }

      /**
       * Get LDI answers
       * @return std::vector<item::AnswerPtr> LDI Answers
       */
      const std::vector<item::AnswerPtr>& getAnswers() const { return m_answers; }
    };
    typedef std::unique_ptr<SendLdiCommands> SendCommandsPtr;

    ////////////////
    class SendLdiCommandsAsync : public SendLdiCommands {
    protected:
      /**
       * Base constructor
       */
      SendLdiCommandsAsync() : SendLdiCommands() {}

      /**
       * Commands constructor
       * @param ldiCommands Vector of LDI commands
       */
      SendLdiCommandsAsync(const std::vector<uint16_t> &ldiCommands) : SendLdiCommands(ldiCommands) {}
    public:
      /**
       * Destructor
       */
      virtual ~SendLdiCommandsAsync() {}
    };
    typedef std::unique_ptr<SendLdiCommands> SendCommandsPtr;

    ////////////////
    class SetLai {
    protected:
      /**
       * Base constructor
       */
      SetLai() {}

      /**
       * Voltage constructor
       * @param voltage Voltage to set
       */
      SetLai(uint16_t voltage) : m_voltage(voltage) {}

      /// Voltage to set
      uint16_t m_voltage = 0;
      /// Previous voltage
      uint16_t m_prevVoltage = 0;
    public:
      /**
       * Destructor
       */
      virtual ~SetLai() {}

      /**
       * Get voltage to set
       * @return uint16_t Voltage to set
       */
      uint16_t getVoltage() const { return m_voltage; }

      /**
       * Get previous voltage
       * @return uint16_t Previous voltage
       */
      uint16_t getPreviousVoltage() const { return m_prevVoltage; }
    };
    typedef std::unique_ptr<SetLai> SetLaiPtr;

    ////////////////
    class FrcLdiSend {
    protected:
      /**
       * Base constructor
       */
      FrcLdiSend() : m_ldiCommand(0) {}

      /**
       * Command constructor
       * @param ldiCommand LDI command
       */
      FrcLdiSend(uint16_t ldiCommand) : m_ldiCommand(ldiCommand) {}

      /**
       * Full constructor
       * @param ldiCommand LDI command
       * @param selectedNodes Selected nodes
       */
      FrcLdiSend(uint16_t ldiCommand, const std::vector<int> &selectedNodes) : m_ldiCommand(ldiCommand), m_selectedNodes(selectedNodes) {}

      /// LDI command to execute
      uint16_t m_ldiCommand;
      /// Selected nodes
      std::vector<int> m_selectedNodes;
      /// Answers
      std::vector<item::AnswerPtr> m_answers;
    public:
      /**
       * Destructor
       */
      virtual ~FrcLdiSend() {}

      /**
       * Get LDI command
       */
      uint16_t getLdiCommand() const { return m_ldiCommand; }

      /**
       * Get selected nodes
       */
      const std::vector<int> &getSelectedNodes() const { return m_selectedNodes; }

      /**
       * Get answers
       * @return std::vector<item::AnswerPtr> Answers
       */
      const std::vector<item::AnswerPtr> &getAnswersPerNode() const { return m_answers; }
    };
    typedef std::unique_ptr<FrcLdiSend> FrcLdiSendPtr;

    ////////////////
    class FrcLaiRead {
    protected:
      /**
       * Base constructor
       */
      FrcLaiRead() {}

      /**
       * Selected nodes constructor
       * @param selectedNodes Selected nodes
       */
      FrcLaiRead(const std::vector<int> &selectedNodes) : m_selectedNodes(selectedNodes) {}

      /// Selected nodes
      std::vector<int> m_selectedNodes;
      /// Voltages
      std::vector<uint16_t> m_voltages;
    public:
      /**
       * Destructor
       */
      virtual ~FrcLaiRead() {}

      /**
       * Get selected nodes
       * @return std::vector<int> Selected nodes
       */
      const std::vector<int>& getSelectedNodes() const { return m_selectedNodes; }

      /**
       * Get node voltages
       * @return std::vector<uint16_t> Node voltages
       */
      const std::vector<uint16_t>& getVoltages() const { return m_voltages; }
    };
    typedef std::unique_ptr<FrcLaiRead> FrcLaiReadPtr;

  } //namespace dali
} //namespace iqrf
