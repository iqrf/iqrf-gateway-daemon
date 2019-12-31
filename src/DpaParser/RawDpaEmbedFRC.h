#pragma once

#include "DpaCommandSolver.h"
#include "EmbedFRC.h"

namespace iqrf
{
  namespace embed
  {
    namespace frc
    {
      namespace rawdpa
      {
        ////////////////
        class Send : public frc::Send, public DpaCommandSolver
        {
        public:
          Send(uint8_t frcCommand, const std::vector<uint8_t> & userData)
            :frc::Send(frcCommand, userData)
            , DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND)
          {}

          Send(uint8_t frcCommand, const std::set<int> & selectedNodes, const std::vector<uint8_t> & userData)
            :frc::Send(frcCommand, selectedNodes, userData)
            , DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND_SELECTIVE)
          {}

          virtual ~Send()
          {}

        protected:
          Send()
            :frc::Send()
            , DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND)
          {}

          Send(const std::set<int> & selectedNodes)
            :frc::Send()
            , DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND_SELECTIVE)
          {
            (void)selectedNodes; //silence -Wunused-parameter
          }

          virtual void encodeRequest(DpaMessage & dpaRequest) override
          {
            if (m_selectedNodes.size() > 0) {
              // fix pcmd according selectedNodes if set later after ctor by setSelectedNodes()
              m_pcmd = CMD_FRC_SEND_SELECTIVE;
              dpaRequest.DpaPacket().DpaRequestPacket_t.PCMD = m_pcmd;

              dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = m_frcCommand;

              // set selectedNodes
              int selNodesLen = sizeof(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes) / sizeof(uint8_t);
              std::vector<uint8_t> snBytes = indexesToBitmap(m_selectedNodes, selNodesLen);
              std::copy(snBytes.data(), snBytes.data() + selNodesLen
                , dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes);

              // set userData
              size_t frcUserDataLen = sizeof(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData) / sizeof(uint8_t);
              if (frcUserDataLen >= m_userData.size()) {
                frcUserDataLen = m_userData.size();
              }
              else {
                TRC_WARNING(PAR(m_userData.size()) << "cut off to: " << PAR(frcUserDataLen));
              }
              std::copy(m_userData.data(), m_userData.data() + frcUserDataLen
                , dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData);

              // set len
              dpaRequest.SetLength(getRequestHeaderLen()
                + sizeof(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand)
                + selNodesLen
                + m_userData.size()
              );
            }
            else {
              m_pcmd = CMD_FRC_SEND;
              dpaRequest.DpaPacket().DpaRequestPacket_t.PCMD = m_pcmd;

              dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = m_frcCommand;

              // set userData
              size_t frcUserDataLen = sizeof(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData) / sizeof(uint8_t);
              if (frcUserDataLen >= m_userData.size()) {
                frcUserDataLen = m_userData.size();
              }
              else {
                TRC_WARNING(PAR(m_userData.size()) << "cut off to: " << PAR(frcUserDataLen));
              }
              std::copy(m_userData.data(), m_userData.data() + frcUserDataLen
                , dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData);

              // set len
              dpaRequest.SetLength(getRequestHeaderLen()
                + sizeof(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand)
                + m_userData.size()
              );
            }
          }

          void parseResponse(const DpaMessage & dpaResponse) override
          {
            m_frcData.clear();
            m_status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
            int frcDataLen = sizeof(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData) / sizeof(uint8_t);
            m_frcData.assign(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData,
              dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData + frcDataLen);
          }
        };
        typedef std::unique_ptr<Send> SendPtr;
        
        ////////////////
        class ExtraResult : public frc::ExtraResult, public DpaCommandSolver
        {
        public:
          ExtraResult()
            :frc::ExtraResult()
            , DpaCommandSolver(0, PNUM_FRC, CMD_FRC_EXTRARESULT)
          {}

          virtual ~ExtraResult()
          {}

        protected:
          void encodeRequest(DpaMessage & dpaRequest) override
          {
            (void)dpaRequest; //silence -Wunused-parameter
          }

          void parseResponse(const DpaMessage & dpaResponse) override
          {
            m_frcData.clear();
            m_frcData.assign(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData,
              dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData + 9);
          }
        };

        ////////////////
        class MemReadBase : public Send
        {
        protected:
          uint16_t m_address;
          uint8_t m_nodePnum;
          uint8_t m_nodePcmd;
          bool m_inc;
          std::vector<uint8_t> m_pdata;
        
          MemReadBase() = delete;

          MemReadBase(uint16_t address, uint8_t pnum, uint8_t pcmd, bool inc, std::vector<uint8_t> pdata )
            :Send()
            , m_address(address)
            , m_nodePnum(pnum)
            , m_nodePcmd(pcmd)
            , m_inc(inc)
            , m_pdata(pdata)
          {
          }

          MemReadBase(const std::set<int> & selectedNodes, uint16_t address, uint8_t pnum, bool inc, uint8_t pcmd, std::vector<uint8_t> pdata)
            :Send(selectedNodes)
            , m_address(address)
            , m_nodePnum(pnum)
            , m_nodePcmd(pcmd)
            , m_inc(inc)
            , m_pdata(pdata)
          {
          }

          virtual ~MemReadBase()
          {}

          void userDataNodeDpa()
          {
            m_userData.push_back((uint8_t)(m_address & 0xff));
            m_userData.push_back((uint8_t)(m_address >> 8));
            m_userData.push_back(m_nodePnum);
            m_userData.push_back(m_nodePcmd);
          }

          void userDataPdata()
          {
            // len according selectedNodes
            size_t maxUserDataLen = m_selectedNodes.size() > 0 ? sizeof(TPerFrcSendSelective_Request::UserData) : sizeof(TPerFrcSend_Request::UserData);

            size_t frcUserDataLen = maxUserDataLen;
            if (frcUserDataLen >= m_pdata.size()) {
              frcUserDataLen = m_pdata.size();
            }
            else {
              TRC_WARNING(PAR(m_pdata.size()) << "cut off to: " << PAR(frcUserDataLen));
            }

            m_userData.push_back((uint8_t)frcUserDataLen);
            m_userData.insert(m_userData.end(), m_pdata.data(), m_pdata.data() + frcUserDataLen);
          }

        };

        ////////////////
        class MemoryRead : public MemReadBase
        {
        public:
          MemoryRead(uint16_t address, uint8_t pnum, uint8_t pcmd, bool inc, std::vector<uint8_t> pdata = std::vector<uint8_t>())
            :MemReadBase(address, pnum, pcmd, inc, pdata)
          {
          }

          MemoryRead(const std::set<int> & selectedNodes, uint16_t address, uint8_t pnum, uint8_t pcmd, bool inc, std::vector<uint8_t> pdata = std::vector<uint8_t>())
            :MemReadBase(selectedNodes, address, pnum, pcmd, inc, pdata)
          {
          }

          void encodeRequest(DpaMessage & dpaRequest) override
          {
            //FRC command according inc
            m_frcCommand = m_inc ? FRC_MemoryReadPlus1 : FRC_MemoryRead;

            m_userData.clear(); //clear to be reusable
            userDataNodeDpa();
            userDataPdata();

            Send::encodeRequest(dpaRequest);
          }

        };

        ////////////////
        class MemoryRead4B : public MemReadBase
        {
        public:
          MemoryRead4B(uint16_t address, uint8_t pnum, uint8_t pcmd, bool inc, std::vector<uint8_t> pdata = std::vector<uint8_t>())
            :MemReadBase(address, pnum, pcmd, inc, pdata)
          {
          }

          MemoryRead4B(const std::set<int> & selectedNodes, uint16_t address, uint8_t pnum, uint8_t pcmd, bool inc, std::vector<uint8_t> pdata = std::vector<uint8_t>())
            :MemReadBase(selectedNodes, address, pnum, pcmd, inc, pdata)
          {
          }

          void encodeRequest(DpaMessage & dpaRequest) override
          {
            m_frcCommand = FRC_MemoryRead4B;

            m_userData.clear(); //clear to be reusable
            m_userData.push_back(m_inc ? 1 : 0); //inc 0 - value original, 1- value += 1
            m_userData.push_back(0); // required zero
            userDataNodeDpa();
            userDataPdata();

            Send::encodeRequest(dpaRequest);
          }

        };

      } //namespace rawdpa
    } //namespace frc
  } //namespace embed
} //namespace iqrf
