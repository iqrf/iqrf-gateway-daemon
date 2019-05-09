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
    //for debug only
    static std::string JsonToStr(const rapidjson::Value* val)
    {
      rapidjson::Document doc;
      doc.CopyFrom(*val, doc.GetAllocator());
      rapidjson::StringBuffer buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      return buffer.GetString();
    }

    // nadr, hwpid not set from drivers
    //TODO return directly DpaMessage to avoid later conversion vector -> DpaMessage
    static std::vector<uint8_t> rawHdpRequestToDpaRequest(int nadr, int hwpid, const std::string& rawHdpRequest)
    {
      using namespace rapidjson;

      std::vector<uint8_t> retvect;

      Document doc;
      doc.Parse(rawHdpRequest);

      uint8_t pnum = 0, pcmd = 0;

      if (Value *val = Pointer("/pnum").Get(doc)) {
        parseHexaNum(pnum, val->GetString());
      }
      if (Value *val = Pointer("/pcmd").Get(doc)) {
        parseHexaNum(pcmd, val->GetString());
      }

      retvect.push_back(nadr & 0xff);
      retvect.push_back((nadr >> 8) & 0xff);
      retvect.push_back(pnum);
      retvect.push_back(pcmd);
      retvect.push_back(hwpid & 0xff);
      retvect.push_back((hwpid >> 8) & 0xff);

      if (Value *val = Pointer("/rdata").Get(doc)) {
        int len = parseBinary(retvect, val->GetString(), 0xFF);
      }

      return retvect;
    }

    // nadr, hwpid, rcode not set for drivers
    //TODO rewrite with const DpaMessage& dpaResponse to avoid previous conversion DpaMessage -> vector
    static std::string dpaResponseToRawHdpResponse(int& nadr, int& hwpid, int& rcode, const std::vector<uint8_t>& dpaResponse, const std::string& rawHdpRequest)
    {
      using namespace rapidjson;

      std::string rawHdpResponse;

      Document doc;

      if (dpaResponse.size() >= 8) {
        uint8_t pnum = 0, pcmd = 0, rcode8 = 0, dpaval = 0;
        std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;

        nadr = dpaResponse[0];
        nadr += dpaResponse[1] << 8;
        pnum = dpaResponse[2];
        pcmd = dpaResponse[3];
        hwpid = dpaResponse[4];
        hwpid += dpaResponse[5] << 8;
        rcode8 = dpaResponse[6];
        rcode = rcode8;
        dpaval = dpaResponse[7];

        pnumStr = encodeHexaNum(pnum);
        pcmdStr = encodeHexaNum(pcmd);
        rcodeStr = encodeHexaNum(rcode8);
        dpavalStr = encodeHexaNum(dpaval);

        //nadr, hwpid is not interesting for drivers
        Pointer("/pnum").Set(doc, pnumStr);
        Pointer("/pcmd").Set(doc, pcmdStr);
        Pointer("/rcode").Set(doc, rcodeStr);
        Pointer("/dpaval").Set(doc, rcodeStr);

        if (dpaResponse.size() > 8) {
          Pointer("/rdata").Set(doc, encodeBinary(dpaResponse.data() + 8, static_cast<int>(dpaResponse.size()) - 8));
        }

        Document rawHdpRequestDoc;
        rawHdpRequestDoc.Parse(rawHdpRequest);
        Pointer("/originalRequest").Set(doc, rawHdpRequestDoc);

        rawHdpResponse = JsonToStr(&doc);
      }

      return rawHdpResponse;
    }
  };
}
