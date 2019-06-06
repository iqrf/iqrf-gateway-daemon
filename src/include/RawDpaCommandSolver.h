#pragma once

#include "DpaCommandSolver.h"

namespace iqrf {

  class RawDpaCommandSolver : public DpaCommandSolver
  {
  protected:
    DpaMessage m_request;

  public:
    virtual ~RawDpaCommandSolver() {}


    RawDpaCommandSolver(uint16_t nadr, uint8_t pnum, uint8_t pcmd)
      :DpaCommandSolver(nadr, pnum, pcmd)
    {
      initRequest();
    }

    RawDpaCommandSolver(uint16_t nadr, uint8_t pnum, uint8_t pcmd, uint16_t hwpid)
      :DpaCommandSolver(nadr, pnum, pcmd, hwpid)
    {
      initRequest();
    }

    //override to encode data
    virtual DpaMessage encodeRequest()
    { 
      return m_request;
    }
    
    //override to parse data
    virtual void parseResponse() = 0;

  private: 
    void initRequest()
    {
      m_request.DpaPacket().DpaRequestPacket_t.NADR = m_nadr;
      m_request.DpaPacket().DpaRequestPacket_t.PNUM = m_pnum;
      m_request.DpaPacket().DpaRequestPacket_t.PCMD = m_pcmd;
      m_request.DpaPacket().DpaRequestPacket_t.HWPID = m_hwpid;
      m_request.SetLength(sizeof(TDpaIFaceHeader));
    }
  };

}
