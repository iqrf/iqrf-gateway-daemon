#pragma once

#include "HexStringCoversion.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/pointer.h"
#include <string>
#include <vector>

namespace iqrf {

  class JsdConversion {
  public:
    static std::string JsonToStr(const rapidjson::Value* val)
    {
      rapidjson::Document doc;
      doc.CopyFrom(*val, doc.GetAllocator());
      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      return buffer.GetString();
    }

    static DpaMessage rawHdpRequestToDpaRequest(int nadr, int hwpid, const std::string& rawHdpRequest)
    {
      using namespace rapidjson;

      DpaMessage retval;

      Document doc;
      doc.Parse(rawHdpRequest);

      uint8_t pnum = 0, pcmd = 0;

      if (Value *val = Pointer("/pnum").Get(doc)) {
        parseHexaNum(pnum, val->GetString());
      }
      if (Value *val = Pointer("/pcmd").Get(doc)) {
        parseHexaNum(pcmd, val->GetString());
      }

      uint8_t* p0 = retval.DpaPacket().Buffer;
      uint8_t* p = p0;

      *p++ = nadr & 0xff;
      *p++ = (nadr >> 8) & 0xff;
      *p++ = pnum;
      *p++ = pcmd;
      *p++ = hwpid & 0xff;
      *p++ = (hwpid >> 8) & 0xff;

      if (Value *val = Pointer("/rdata").Get(doc)) {
        int len = parseBinary(p, val->GetString(), 0xFF);
        p += len;
      }

      retval.SetLength(p - p0);

      return retval;
    }

    static std::string dpaResponseToRawHdpResponse(int& nadr, int& hwpid, int& rcode, const DpaMessage & dpaResponse, const std::string & rawHdpRequest)
    {
      using namespace rapidjson;

      std::string rawHdpResponse;
      Document doc;

      const uint8_t* p = dpaResponse.DpaPacket().Buffer;
      int len = dpaResponse.GetLength();

      if (len >= 8) {
        uint8_t pnum = 0, pcmd = 0, rcode8 = 0, dpaval = 0;
        std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;

        nadr = p[0];
        nadr += p[1] << 8;
        pnum = p[2];
        pcmd = p[3];
        hwpid = p[4];
        hwpid += p[5] << 8;
        rcode8 = p[6];
        rcode = rcode8;
        dpaval = p[7];

        pnumStr = encodeHexaNum(pnum);
        pcmdStr = encodeHexaNum(pcmd);
        rcodeStr = encodeHexaNum(rcode8);
        dpavalStr = encodeHexaNum(dpaval);

        //nadr, hwpid is not interesting for drivers
        Pointer("/pnum").Set(doc, pnumStr);
        Pointer("/pcmd").Set(doc, pcmdStr);
        Pointer("/rcode").Set(doc, rcodeStr);
        Pointer("/dpaval").Set(doc, rcodeStr);

        if (len > 8) {
          Pointer("/rdata").Set(doc, encodeBinary(p + 8, static_cast<int>(len) - 8));
        }

        if (rawHdpRequest.size() > 0) {
          Document rawHdpRequestDoc;
          rawHdpRequestDoc.Parse(rawHdpRequest);
          Pointer("/originalRequest").Set(doc, rawHdpRequestDoc);
        }

        rawHdpResponse = JsonToStr(&doc);
      }

      return rawHdpResponse;
    }

  };
}
