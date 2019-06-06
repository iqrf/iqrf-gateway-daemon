#pragma once

#include "IDpaTransactionResult2.h"
#include "Trace.h"
#include "stdint.h"
#include <memory>
#include <vector>

namespace iqrf {

  class DpaCommandSolver
  {
  protected:
    uint16_t m_nadr;
    uint8_t m_pnum;
    uint8_t m_pcmd;
    uint16_t m_hwpid;
    uint8_t m_rcode;
    uint8_t m_dpaval;
    std::vector<uint8_t> m_rdata;

    std::unique_ptr<IDpaTransactionResult2> m_dpaTransactionResult2;

  public:
    virtual ~DpaCommandSolver() {}

    DpaCommandSolver() = delete;

    DpaCommandSolver(uint16_t nadr)
      :m_nadr(nadr)
      , m_pnum(0)
      , m_pcmd(0)
      , m_hwpid(0xFFFF)
      , m_rcode(0)
      , m_dpaval(0)
    {}

    DpaCommandSolver(uint16_t nadr, uint16_t hwpid)
      :m_nadr(nadr)
      , m_pnum(0)
      , m_pcmd(0)
      , m_hwpid(hwpid)
      , m_rcode(0)
      , m_dpaval(0)
    {}

    DpaCommandSolver(uint16_t nadr, uint8_t pnum, uint8_t pcmd)
      :m_nadr(nadr)
      , m_pnum(pnum)
      , m_pcmd(pcmd)
      , m_hwpid(0xFFFF)
      , m_rcode(0)
      , m_dpaval(0)
    {}

    DpaCommandSolver(uint16_t nadr, uint8_t pnum, uint8_t pcmd, uint16_t hwpid)
      :m_nadr(nadr)
      , m_pnum(pnum)
      , m_pcmd(pcmd)
      , m_hwpid(hwpid)
      , m_rcode(0)
      , m_dpaval(0)
    {}

    uint16_t getNadr() const { return m_nadr; }
    uint16_t getHwpid() const { return m_hwpid; }
    uint8_t getPnum() const { return m_pnum; }
    uint8_t getPcmd() const { return m_pcmd; }
    uint8_t getRcode() const { return m_rcode; }
    uint8_t getDpaval() const { return m_dpaval; }
    const std::vector<uint8_t> & getRdata() const { return m_rdata; }

    const std::unique_ptr<IDpaTransactionResult2> & getResult() const
    {
      return m_dpaTransactionResult2;
    }
    
    void processResult(std::unique_ptr<IDpaTransactionResult2> res)
    {
      m_dpaTransactionResult2 = std::move(res);

      if (!m_dpaTransactionResult2->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No response");
      }

      const DpaMessage & dpaResponse = m_dpaTransactionResult2->getResponse();
      const uint8_t* p = dpaResponse.DpaPacket().Buffer;
      int len = dpaResponse.GetLength();

      if (len < 8) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid dpaResponse");
      }

      uint16_t nadr = p[0];
      nadr += p[1] << 8;

      if (nadr != m_nadr) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid nadr:" << NAME_PAR(expected, m_nadr) << NAME_PAR(delivered, nadr));
      }

      //TODO check pnum, pcmd

      m_pnum = p[2];
      m_pcmd = p[3];
      m_hwpid = p[4];
      m_hwpid += p[5] << 8;
      m_rcode = p[6];
      m_dpaval = p[7];

      if (0 != m_rcode) {
        THROW_EXC_TRC_WAR(std::logic_error, "Bad response: " << NAME_PAR(rcode, m_rcode));
      }

      if (len > 8) {
        m_rdata = std::vector<uint8_t>(p + 8, p + static_cast<int>(len));
      }

    }
  };
}
