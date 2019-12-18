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

        enum class FrcCmdType {
          FRC_BIT = 0,
          FRC_BYTE = 1,
          FRC_2BYTE = 2,
          FRC_4BYTE = 4
        };

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

        std::vector<std::set<int>> splitSelectedNode(const std::set<int> & selectedNodeAll)
        {
          size_t cnt = 0;
          switch (getFrcCmdType()) {
          case FrcCmdType::FRC_BIT: cnt = 239;
            break;
          case FrcCmdType::FRC_BYTE: cnt = 63;
            break;
          case FrcCmdType::FRC_2BYTE: cnt = 31;
            break;
          case FrcCmdType::FRC_4BYTE: cnt = 15;
            break;
          default:;
          }

          std::vector<std::set<int>> resVect;
          size_t resVectSize = selectedNodeAll.size() / cnt;
          size_t remains = selectedNodeAll.size() % cnt;
          auto snIt = selectedNodeAll.begin();

          for (size_t sz = 0; sz <= resVectSize; sz++) {
            std::set<int> resSet;
            if (sz != resVectSize) {
              //resSet.insert(snIt, snIt + cnt);
              resSet.insert(snIt, std::next(snIt, cnt));
              std::advance(snIt, cnt);
            }
            else {
              // remains
              resSet.insert(snIt, std::next(snIt, remains));
            }

            resVect.push_back(resSet);
          }
          return resVect;
        }

        FrcCmdType getFrcCmdType()
        {
          if (m_frcCommand < FRC_USER_BIT_TO) return FrcCmdType::FRC_BIT;
          if (m_frcCommand < FRC_USER_BYTE_TO) return FrcCmdType::FRC_BYTE;
          if (m_frcCommand < FRC_USER_2BYTE_TO) return FrcCmdType::FRC_2BYTE;
          if (m_frcCommand < FRC_USER_4BYTE_TO) return FrcCmdType::FRC_4BYTE;
        }

      };
      typedef std::unique_ptr<Send> SendPtr;

      ////////////////
      class SendSelective : public Send
      {
      protected:
        //params
        std::set<int> m_selectedNodes;

        SendSelective()
          :Send()
        {}

        SendSelective(uint8_t frcCommand, const std::vector<uint8_t> & userData)
          :Send(frcCommand, userData)
        {}

        SendSelective(const std::set<int> & selectedNodes)
          :Send()
          , m_selectedNodes(selectedNodes)
        {}

        SendSelective(uint8_t frcCommand, const std::set<int> & selectedNodes, const std::vector<uint8_t> & userData)
          :Send(frcCommand, userData)
          , m_selectedNodes(selectedNodes)
        {}

      public:
        virtual ~SendSelective() {}

        virtual std::set<int> getSelectedNodes() override { return m_selectedNodes; }

        void setSelectedNodes(const std::set<int> & val) { m_selectedNodes = val; }

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
