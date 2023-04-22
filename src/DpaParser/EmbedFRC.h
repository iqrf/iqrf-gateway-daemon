/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
      ////////////////
      class Send
      {
      protected:
        //params
        uint8_t m_frcCommand;
        std::vector<uint8_t> m_userData;
        std::set<uint8_t> m_selectedNodes;

        //response
        int m_status;
        std::vector<uint8_t> m_frcData;

        Send(uint8_t frcCommand, const std::vector<uint8_t> & userData)
          :m_frcCommand(frcCommand)
          , m_userData(userData)
          , m_status(0)
        {}

        Send(uint8_t frcCommand, const std::set<uint8_t> &selectedNodes, const std::vector<uint8_t> &userData)
          :m_frcCommand(frcCommand)
          , m_userData(userData)
          , m_selectedNodes(selectedNodes)
          , m_status(0)
        {}

        Send()
          :m_frcCommand(0xFF)
          , m_status(0)
        {}

        Send(const std::set<uint8_t> &selectedNodes)
          :m_frcCommand(0xFF)
          , m_selectedNodes(selectedNodes)
          , m_status(0)
        {}

      public:
        virtual ~Send() {}

        // get param data passes by ctor
        uint8_t getFrcCommand() const { return m_frcCommand; };
        const std::vector<uint8_t> & getUserData() const { return m_userData; }

        // get data as returned from frc
        uint8_t getStatus() const { return (uint8_t)m_status; }
        const std::vector<uint8_t> & getFrcData() const { return m_frcData; }

        // get selected nodes
        const std::set<uint8_t> & getSelectedNodes() const { return m_selectedNodes; }

        // makes selected cmd
        void setSelectedNodes(const std::set<uint8_t> &val)
        {
          m_selectedNodes = val;
        }

        // typed data parser according selected nadrs
        // in case of non-selective it fill nadrs row according type
        template <typename T>
        void getFrcDataAs(std::map<int, T> & map2Insert, const std::vector<uint8_t> & extResult = std::vector<uint8_t>() )
        {
          std::set<int> selectedNodes = m_selectedNodes;
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
            selectedNodes.insert(0); //just to cope with fact the FRC always returns result for 0
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

          map2Insert.erase(map2Insert.begin()); // erase useless 0 item
        }

        template <typename T>
        static std::vector<std::set<int>> splitSelectedNode(const std::set<int> & selectedNodeAll)
        {
          std::set<int> localSelectedNodeAll(selectedNodeAll);
          localSelectedNodeAll.erase(0); // useless 0 -coordinator is not evaluated

          size_t cnt = 0;
          switch (sizeof(T)) {
          case 1: cnt = 63;
            break;
          case 2: cnt = 31;
            break;
          case 4: cnt = 15;
            break;
          default:;
          }

          std::vector<std::set<int>> resVect;
          size_t resVectSize = localSelectedNodeAll.size() / cnt;
          size_t remains = localSelectedNodeAll.size() % cnt;
          auto snIt = localSelectedNodeAll.begin();

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

            if (resSet.size() > 0)
              resVect.push_back(resSet);
          }
          return resVect;
        }

      };
      typedef std::unique_ptr<Send> SendPtr;

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
