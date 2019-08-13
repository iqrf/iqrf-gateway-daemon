#pragma once

#include <string>
#include <set>
#include <vector>
#include <memory>

namespace iqrf
{
  namespace embed
  {
    namespace frc
    {
      //TODO enum commands?
      
      ////////////////
      class Send
      {
      protected:
        //params
        uint8_t m_frcCommand;
        std::vector<uint8_t> m_userData;

        //response
        int m_status;
        std::vector<int> m_frcData;

        Send()
          :m_frcCommand(0)
          , m_status(0)
        {}

        Send(uint8_t frcCommand, const std::vector<uint8_t> & userData)
          :m_frcCommand(frcCommand)
          , m_userData(userData)
          , m_status(0)
        {}

      public:
        virtual ~Send() {}

        // get param data passes by ctor
        uint8_t getFrcCommand() const { m_frcCommand; };
        const std::vector<uint8_t> & getUserData() const { return m_userData; }

        // get data as returned from driver
        uint8_t getStatus() const { return m_status; }
        const std::vector<int> & getFrcData() const { return m_frcData; }

      };
      typedef std::unique_ptr<Send> SendPtr;

      ////////////////
      class SendSelective : public Send
      {
      protected:
        //params
        std::vector<int> m_selectedNodes;

        //response

        SendSelective()
          :Send()
        {}

        SendSelective(uint8_t frcCommand, const std::vector<int> & selectedNodes, const std::vector<uint8_t> & userData)
          :Send(frcCommand, userData)
          , m_selectedNodes(selectedNodes)
        {}

      public:
        virtual ~SendSelective() {}

        // get param data passes by ctor
        const std::vector<int> & getSelectedNodes() const { return m_selectedNodes; }

      };
      typedef std::unique_ptr<SendSelective> SendSelectivePtr;

      ////////////////
      class ExtraResult
      {
      protected:
        //params

        //response
        std::vector<int> m_frcData;

        ExtraResult()
        {}

      public:
        virtual ~ExtraResult() {}

        // get data as returned from driver
        const std::vector<int> & getFrcData() const { return m_frcData; }

      };
      typedef std::unique_ptr<ExtraResult> ExtraResultPtr;

    } //namespace frc
  } //namespace embed
} //namespace iqrf
