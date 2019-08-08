#pragma once

#include "IDpaTransactionResult2.h"
#include "Trace.h"
#include "stdint.h"
#include <memory>
#include <vector>
#include <set>

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
    bool m_asyncResponse;
    std::vector<uint8_t> m_rdata;
    std::unique_ptr<IDpaTransactionResult2> m_dpaTransactionResult2;
    DpaMessage m_request;

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
      , m_asyncResponse(false)
    {}

    DpaCommandSolver(uint16_t nadr, uint16_t hwpid)
      :m_nadr(nadr)
      , m_pnum(0)
      , m_pcmd(0)
      , m_hwpid(hwpid)
      , m_rcode(0)
      , m_dpaval(0)
      , m_asyncResponse(false)
    {}

    DpaCommandSolver(uint16_t nadr, uint8_t pnum, uint8_t pcmd)
      :m_nadr(nadr)
      , m_pnum(pnum)
      , m_pcmd(pcmd)
      , m_hwpid(0xFFFF)
      , m_rcode(0)
      , m_dpaval(0)
      , m_asyncResponse(false)
    {}

    DpaCommandSolver(uint16_t nadr, uint8_t pnum, uint8_t pcmd, uint16_t hwpid)
      :m_nadr(nadr)
      , m_pnum(pnum)
      , m_pcmd(pcmd)
      , m_hwpid(hwpid)
      , m_rcode(0)
      , m_dpaval(0)
      , m_asyncResponse(false)
    {}

    uint16_t getNadr() const { return m_nadr; }
    uint16_t getHwpid() const { return m_hwpid; }
    uint8_t getPnum() const { return m_pnum; }
    uint8_t getPcmd() const { return m_pcmd; }
    uint8_t getRcode() const { return m_rcode; }
    bool isAsyncRcode() const { return m_asyncResponse; }
  
    uint8_t getDpaval() const { return m_dpaval; }
    const std::vector<uint8_t> & getRdata() const { return m_rdata; }

    const std::unique_ptr<IDpaTransactionResult2> & getResult() const
    {
      return m_dpaTransactionResult2;
    }
    
    std::unique_ptr<IDpaTransactionResult2> getResultMove()
    {
      return std::move(m_dpaTransactionResult2);
    }

    virtual DpaMessage encodeRequest() = 0;

    void processAsyncResponse(const DpaMessage & dpaResponse)
    {
      processResponse(dpaResponse);
      if (!isAsyncRcode()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid async response code:"
          << NAME_PAR(expected, (int)STATUS_ASYNC_RESPONSE) << NAME_PAR(delivered, getRcode()));
      }
    }

    void processDpaTransactionResult(std::unique_ptr<IDpaTransactionResult2> res)
    {
      m_dpaTransactionResult2 = std::move(res);

      if (!m_dpaTransactionResult2->isResponded()) {
        THROW_EXC_TRC_WAR(std::logic_error, "No response");
      }

      const DpaMessage & dpaResponse = m_dpaTransactionResult2->getResponse();
      processResponse(dpaResponse);
    }

    static std::set<int> bitmapToIndexes(const uint8_t* bitmap, int indexFrom, int indexTo, int offset)
    {
      std::set<int> retval;

      for (int index = indexFrom; index <= indexTo; index++) {
        unsigned bitmapByte = bitmap[index];
        if (0 == bitmapByte) {
          offset += 8;
        }
        else {
          for (unsigned bitMask = 0x01; bitMask != 0x100; bitMask <<= 1) {
            if ((bitmapByte & bitMask) != 0) {
              retval.insert(offset);
            }
            offset++;
          }
        }
      }
      return retval;
    }

  protected:
    virtual void parseResponse(const DpaMessage & dpaResponse) = 0;

    void initRequestHeader(DpaMessage & request) const
    {
      request.DpaPacket().DpaRequestPacket_t.NADR = m_nadr;
      request.DpaPacket().DpaRequestPacket_t.PNUM = m_pnum;
      request.DpaPacket().DpaRequestPacket_t.PCMD = m_pcmd;
      request.DpaPacket().DpaRequestPacket_t.HWPID = m_hwpid;
      request.SetLength(sizeof(TDpaIFaceHeader));
    }

  private:
    void processResponse(const DpaMessage & dpaResponse)
    {
      int len = dpaResponse.GetLength();

      if (len < sizeof(TDpaIFaceHeader) || len > sizeof(TDpaIFaceHeader) + 2 + DPA_MAX_DATA_LENGTH) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid dpaResponse lenght: " << PAR(len));
      }

      const auto & rp = dpaResponse.DpaPacket().DpaResponsePacket_t;

      uint16_t nadr = rp.NADR;
      if (nadr != m_nadr) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid nadr:" << NAME_PAR(expected, (int)m_nadr) << NAME_PAR(delivered, (int)nadr));
      }

      uint8_t pnum = rp.PNUM;
      if (pnum != m_pnum) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid pnum:" << NAME_PAR(expected, (int)m_pnum) << NAME_PAR(delivered, (int)pnum));
      }

      uint8_t pcmd = rp.PCMD & 0x7F; // mask highest bit set by response
      if (pcmd != m_pcmd) {
        THROW_EXC_TRC_WAR(std::logic_error, "Invalid pnum:" << NAME_PAR(expected, (int)m_pcmd) << NAME_PAR(delivered, (int)pcmd));
      }

      m_hwpid = rp.HWPID;
      m_rcode = rp.ResponseCode;
      m_asyncResponse = (m_rcode & STATUS_ASYNC_RESPONSE) != 0;
      m_rcode &= (uint8_t)0x7F;
      m_dpaval = rp.DpaValue;

      if (0 != m_rcode) {
        THROW_EXC_TRC_WAR(std::logic_error, "Bad response: " << NAME_PAR(rcode, m_rcode));
      }

      if (len > sizeof(TDpaIFaceHeader) + 2) { //+ rcode + dpaval
        m_rdata = std::vector<uint8_t>(rp.DpaMessage.Response.PData, rp.DpaMessage.Response.PData + static_cast<int>(len) - sizeof(TDpaIFaceHeader) - 2);
      }

      parseResponse(dpaResponse);
    }

  };
}
