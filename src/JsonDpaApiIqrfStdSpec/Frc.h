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

      public:
        Send(uint8_t frcCommand, const std::vector<uint8_t> & userData)
          :m_frcCommand(frcCommand)
          , m_userData(userData)
          , m_status(0)
        {}

        virtual ~Send() {}

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

      public:
        SendSelective(uint8_t frcCommand, const std::vector<int> & selectedNodes, const std::vector<uint8_t> & userData)
          :Send(frcCommand, userData)
          , m_selectedNodes(selectedNodes)
        {}

        virtual ~SendSelective() {}

      };
      typedef std::unique_ptr<SendSelective> SendSelectivePtr;

      ////////////////
      class ExtraResult
      {
      protected:
        //params

        //response
        std::vector<int> m_remainFrcData;

      public:
        ExtraResult()
        {}

        virtual ~ExtraResult() {}

        // get data as returned from driver
        const std::vector<int> & getRemainFrcData() const { return m_remainFrcData; }

      };
      typedef std::unique_ptr<ExtraResult> ExtraResultPtr;

    } //namespace frc
  } //namespace embed
} //namespace iqrf
