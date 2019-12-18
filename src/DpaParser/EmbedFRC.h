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
        std::vector<uint8_t> m_frcData;

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

        // get data as returned from frc
        uint8_t getStatus() const { return m_status; }
        const std::vector<uint8_t> & getFrcData() const { return m_frcData; }

        // get selected nodes => empty here created according type in getFrcDataAs
        virtual std::set<int> getSelectedNodes() { return std::set<int>(); }

        // typed data parser according selected nadrs
        // in case of non-selective it fill nadrs row according type
        template <typename T>
        void getFrcDataAs(
          std::map<int, T> & map2Insert
          , const std::vector<uint8_t> & extResult = std::vector<uint8_t>()
        )
        {
          std::set<int> selectedNodes = getSelectedNodes();
          if (0 == selectedNodes.size()) {
            switch (sizeof(T)) {
            case 1: for (int i = 0; i < 64; i++) selectedNodes.insert(i);
              break;
            case 2: for (int i = 0; i < 32; i++) selectedNodes.insert(i);
              break;
            case 4: for (int i = 0; i < 16; i++) selectedNodes.insert(i);
            default:;
            }
          }
          else {
            selectedNodes.insert(0); //FRC always returns result for 0
          }

          m_frcData.insert(m_frcData.end(), extResult.begin(), extResult.end());

          auto it = m_frcData.begin();
          auto sit = selectedNodes.begin();
          size_t typelen = sizeof(T);
          size_t reslen = m_frcData.size() / typelen;

          for (size_t rl = 0; rl < reslen; rl++) {
            if (sit == selectedNodes.end()) break;

            uint32_t val = 0;
            for (size_t tl = 0; tl < typelen; tl++) {
              val |= ((uint32_t)*it++) << tl * 8;
            }

            map2Insert.insert(std::make_pair(*sit++, val));
          }
        }

      };
      typedef std::unique_ptr<Send> SendPtr;

      ////////////////
      class SendSelective : public Send
      {
      protected:
        //params
        std::set<int> m_selectedNodes;

        SendSelective(uint8_t frcCommand, const std::set<int> & selectedNodes, const std::vector<uint8_t> & userData)
          :Send(frcCommand, userData)
          , m_selectedNodes(selectedNodes)
        {}

        SendSelective(const std::set<int> & selectedNodes)
          :Send()
          , m_selectedNodes(selectedNodes)
        {}

      public:
        virtual ~SendSelective() {}

        virtual std::set<int> getSelectedNodes() override { return m_selectedNodes; }

      };
      typedef std::unique_ptr<SendSelective> SendSelectivePtr;

      ////////////////
      class ExtraResult : public Send
      {
      protected:
        ExtraResult()
        {}

      public:
        virtual ~ExtraResult() {}
      };
      typedef std::unique_ptr<ExtraResult> ExtraResultPtr;

    } //namespace frc
  } //namespace embed
} //namespace iqrf
