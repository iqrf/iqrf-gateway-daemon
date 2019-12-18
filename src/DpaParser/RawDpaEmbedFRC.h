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
            :DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND)
            , frc::Send(frcCommand, userData)
          {}

          virtual ~Send()
          {}

        protected:
          Send()
            :DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND)
            , frc::Send()
          {}

          void encodeRequest(DpaMessage & dpaRequest) override
          {
            dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.FrcCommand = m_frcCommand;

            // set userData
            int frcUserDataLen = sizeof(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSend_Request.UserData) / sizeof(uint8_t);
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
        class SendSelective : public frc::SendSelective, public DpaCommandSolver
        {
        public:
          SendSelective(uint8_t frcCommand, const std::set<int> & selectedNodes, const std::vector<uint8_t> & userData)
            :DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND_SELECTIVE)
            , frc::SendSelective(frcCommand, selectedNodes, userData)
          {}

          SendSelective(uint8_t frcCommand, const std::vector<uint8_t> & userData)
            :DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND_SELECTIVE)
            , frc::SendSelective(frcCommand, userData)
          {}

          virtual ~SendSelective()
          {}

        protected:
          SendSelective()
            :DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND_SELECTIVE)
            , frc::SendSelective()
          {}

          SendSelective(const std::set<int> & selectedNodes)
            :DpaCommandSolver(0, PNUM_FRC, CMD_FRC_SEND_SELECTIVE)
            , frc::SendSelective(selectedNodes)
          {}

          void encodeRequest(DpaMessage & dpaRequest) override
          {
            dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = m_frcCommand;

            // set userData
            int frcUserDataLen = sizeof(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData) / sizeof(uint8_t);
            if (frcUserDataLen >= m_userData.size()) {
              frcUserDataLen = m_userData.size();
            }
            else {
              TRC_WARNING(PAR(m_userData.size()) << "cut off to: " << PAR(frcUserDataLen));
            }
            std::copy(m_userData.data(), m_userData.data() + frcUserDataLen
              , dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData);

            // set selectedNodes
            int selNodesLen = sizeof(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes) / sizeof(uint8_t);
            std::vector<uint8_t> snBytes = indexesToBitmap(m_selectedNodes, selNodesLen);
            std::copy(snBytes.data(), snBytes.data() + selNodesLen
              , dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes);
          
            // set len
            dpaRequest.SetLength(getRequestHeaderLen()
              + sizeof(dpaRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand)
              + selNodesLen
              + m_userData.size()
            );

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
        typedef std::unique_ptr<SendSelective> SendSelectivePtr;

        ////////////////
        class ExtraResult : public frc::ExtraResult, public DpaCommandSolver
        {
        public:
          ExtraResult()
            :DpaCommandSolver(0, PNUM_FRC, CMD_FRC_EXTRARESULT)
            , frc::ExtraResult()
          {}

          virtual ~ExtraResult()
          {}

        protected:
          void encodeRequest(DpaMessage & dpaRequest) override
          {
          }

          void parseResponse(const DpaMessage & dpaResponse) override
          {
            m_frcData.clear();
            m_frcData.assign(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData,
              dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData + 9);
          }
        };

        ////////////////
        class MemoryRead_Send : public Send
        {
        public:
          MemoryRead_Send(uint16_t address, uint8_t pnum, uint8_t pcmd, std::vector<uint8_t> pdata = std::vector<uint8_t>())
            :Send()
          {
            m_frcCommand = FRC_MemoryRead;
            initUserData(m_userData, address, pnum, pcmd, pdata, (sizeof(TPerFrcSend_Request::UserData) / sizeof(uint8_t)));
          }

          static void initUserData(std::vector<uint8_t> & userData, uint16_t address, uint8_t pnum, uint8_t pcmd
            , const std::vector<uint8_t> & pdata, int maxUserDataLen)
          {
            userData.push_back((uint8_t)(address & 0xff));
            userData.push_back((uint8_t)(address >> 8));
            userData.push_back(pnum);
            userData.push_back(pcmd);
            userData.push_back(pcmd);

            // set userData
            int frcUserDataLen = maxUserDataLen;
            if (frcUserDataLen >= pdata.size()) {
              frcUserDataLen = pdata.size();
            }
            else {
              TRC_WARNING(PAR(pdata.size()) << "cut off to: " << PAR(frcUserDataLen));
            }
            
            userData.push_back(frcUserDataLen);
            userData.insert(userData.end(), pdata.data(), pdata.data() + frcUserDataLen);
          }
        };

        ////////////////
        class MemoryRead_SendSelective : public SendSelective
        {
        public:
          MemoryRead_SendSelective(const std::set<int> & selectedNodes, uint16_t address, uint8_t pnum, uint8_t pcmd
            , std::vector<uint8_t> pdata = std::vector<uint8_t>()
          )
            :SendSelective(selectedNodes)
          {
            m_frcCommand = FRC_MemoryRead;
            MemoryRead_Send::initUserData(m_userData, address, pnum, pcmd, pdata, (sizeof(TPerFrcSendSelective_Request::UserData) / sizeof(uint8_t)));
          }
        };

        ////////////////
        class MemoryRead4B_Send : public Send
        {
        public:
          MemoryRead4B_Send(uint16_t address, uint8_t pnum, uint8_t pcmd, std::vector<uint8_t> pdata = std::vector<uint8_t>())
            :Send()
          {
            m_frcCommand = FRC_MemoryRead4B;
            m_userData.push_back(0); //original
            m_userData.push_back(0); // required zero
            MemoryRead_Send::initUserData(m_userData, address, pnum, pcmd, pdata, (sizeof(TPerFrcSend_Request::UserData) / sizeof(uint8_t)));
          }
        };

        ////////////////
        class MemoryRead4B_SendSelective : public SendSelective
        {
        public:
          MemoryRead4B_SendSelective(const std::set<int> & selectedNodes, uint16_t address, uint8_t pnum, uint8_t pcmd
            , std::vector<uint8_t> pdata = std::vector<uint8_t>()
          )
            :SendSelective(selectedNodes)
          {
            m_frcCommand = FRC_MemoryRead4B;
            m_userData.push_back(0); //original
            m_userData.push_back(0); // required zero
            MemoryRead_Send::initUserData(m_userData, address, pnum, pcmd, pdata, (sizeof(TPerFrcSendSelective_Request::UserData) / sizeof(uint8_t)));
          }

          MemoryRead4B_SendSelective(uint16_t address, uint8_t pnum, uint8_t pcmd
            , std::vector<uint8_t> pdata = std::vector<uint8_t>()
          )
            :SendSelective()
          {
            m_frcCommand = FRC_MemoryRead4B;
            m_userData.push_back(0); //original
            m_userData.push_back(0); // required zero
            MemoryRead_Send::initUserData(m_userData, address, pnum, pcmd, pdata, (sizeof(TPerFrcSendSelective_Request::UserData) / sizeof(uint8_t)));
          }

        };

      } //namespace rawdpa
    } //namespace frc
  } //namespace embed
} //namespace iqrf
