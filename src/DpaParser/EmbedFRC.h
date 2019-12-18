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

        virtual std::map<int, uint8_t> getFrcDataAs1B(std::vector<uint8_t> extResult = std::vector<uint8_t>()) 
        {
          m_frcData.insert(m_frcData.end(), extResult.begin(), extResult.end());
          int i = 0;
          std::map<int, uint8_t> ret;
          auto it = m_frcData.begin();
          
          while (true) {
            if (it == m_frcData.end()) break;
            ret.insert(std::make_pair(i++, *it++));
          }
          return ret;
        }

        virtual std::map<int, uint16_t> getFrcDataAs2B(std::vector<uint8_t> extResult = std::vector<uint8_t>())
        {
          m_frcData.insert(m_frcData.end(), extResult.begin(), extResult.end());
          int i = 0;
          std::map<int, uint16_t> ret;
          auto it = m_frcData.begin();
          
          while (true) {
            if (it == m_frcData.end()) break;
            uint16_t val = (uint16_t)*it++;

            if (it == m_frcData.end()) break;
            val |= ((uint16_t)*it++) << 8;

            ret.insert(std::make_pair(i++, val));
          }
          return ret;
        }

        virtual std::map<int, uint32_t> getFrcDataAs4B(std::vector<uint8_t> extResult = std::vector<uint8_t>())
        {
          m_frcData.insert(m_frcData.end(), extResult.begin(), extResult.end());
          int i = 0;
          std::map<int, uint32_t> ret;
          auto it = m_frcData.begin();
          
          while (true) {
            if (it == m_frcData.end()) break;
            uint32_t val = (uint32_t)*it++;

            if (it == m_frcData.end()) break;
            val |= ((uint32_t)*it++) << 8;

            if (it == m_frcData.end()) break;
            val |= ((uint32_t)*it++) << 16;

            if (it == m_frcData.end()) break;
            val |= ((uint32_t)*it++) << 24;

            ret.insert(std::make_pair(i++, val));
          }
          return ret;
        }

      public:
        virtual ~Send() {}

        // get param data passes by ctor
        uint8_t getFrcCommand() const { m_frcCommand; };
        const std::vector<uint8_t> & getUserData() const { return m_userData; }

        // get data as returned from driver
        uint8_t getStatus() const { return m_status; }
        const std::vector<uint8_t> & getFrcData() const { return m_frcData; }

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

        // get param data passes by ctor
        const std::set<int> & getSelectedNodes() const { return m_selectedNodes; }

        virtual std::map<int, uint8_t> getFrcDataAs1B(std::vector<uint8_t> extResult = std::vector<uint8_t>())
        {
          m_frcData.insert(m_frcData.end(), extResult.begin(), extResult.end());
          std::map<int, uint8_t> ret;
          auto it = m_frcData.begin();
          auto sit = m_selectedNodes.begin();
          
          while (true) {
            if (sit == m_selectedNodes.end()) break;
            
            if (it == m_frcData.end()) break;
            ret.insert(std::make_pair(*sit++, *it++));
          }
          return ret;
        }

        virtual std::map<int, uint16_t> getFrcDataAs2B(std::vector<uint8_t> extResult = std::vector<uint8_t>())
        {
          m_frcData.insert(m_frcData.end(), extResult.begin(), extResult.end());
          std::map<int, uint16_t> ret;
          auto it = m_frcData.begin();
          auto sit = m_selectedNodes.begin();
          
          while (true) {
            if (sit == m_selectedNodes.end()) break;
            
            if (it == m_frcData.end()) break;
            uint16_t val = (uint16_t)*it++;

            if (it == m_frcData.end()) break;
            val |= ((uint16_t)*it++) << 8;

            ret.insert(std::make_pair(*sit++, *it));
          }
          return ret;
        }

        virtual std::map<int, uint32_t> getFrcDataAs4B(std::vector<uint8_t> extResult = std::vector<uint8_t>())
        {
          std::vector<int> pom;
          pom.insert(pom.begin(), m_frcData.begin(), m_frcData.end());

          m_frcData.insert(m_frcData.end(), extResult.begin(), extResult.end());
          std::map<int, uint32_t> ret;
          auto it = m_frcData.begin();
          auto sit = m_selectedNodes.begin();

          while (true) {
            if (sit == m_selectedNodes.end()) break;

            if (it == m_frcData.end()) break;
            uint32_t val = (uint32_t)*it++;

            if (it == m_frcData.end()) break;
            val |= ((uint32_t)*it++) << 8;

            if (it == m_frcData.end()) break;
            val |= ((uint32_t)*it++) << 16;

            if (it == m_frcData.end()) break;
            val |= ((uint32_t)*it++) << 24;

            ret.insert(std::make_pair(*sit++, val));
          }
          return ret;
        }

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
