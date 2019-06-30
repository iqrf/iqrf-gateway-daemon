#include "JsDriverService.h"
#include "HexStringCoversion.h"

#include "Trace.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"
#include "rapidjson/writer.h"

#include "iqrf__JsDriverService.hxx"

TRC_INIT_MODULE(iqrf::JsDriverService);

namespace iqrf {

  // implementation class
  class JsDriverService::Imp
  {
  private:
    iqrf::IJsRenderService* m_iJsRenderService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    DpaMessage createDpaRequest(JsDriverDpaCommandSolver & jsd)
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      std::string functionNameReq(jsd.functionName());
      functionNameReq += "_Request_req";

      // call request driver func, it returns rawHdpRequest format in text form
      try {
        m_iJsRenderService->callFenced(jsd.getNadr(), functionNameReq, jsd.requestParameter(), jsd.storeRequest());
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver request failure: ");
        //TODO special request error exc
        THROW_EXC_TRC_WAR(std::logic_error, "Driver request failure: " << e.what());
      }

      TRC_DEBUG(PAR(jsd.storeRequest()));

      // convert from rawHdpRequest to dpaRequest and pass nadr and hwpid to be in dapaRequest (driver doesn't set them)
      Document doc;
      doc.Parse(jsd.storeRequest());

      uint8_t pnum = 0, pcmd = 0;

      if (Value *val = Pointer("/pnum").Get(doc)) {
        parseHexaNum(pnum, val->GetString());
      }
      if (Value *val = Pointer("/pcmd").Get(doc)) {
        parseHexaNum(pcmd, val->GetString());
      }

      DpaMessage dpaRequest;

      uint8_t* p0 = dpaRequest.DpaPacket().Buffer;
      uint8_t* p = p0;
      uint16_t nadr = jsd.getNadr();
      uint16_t hwpid = jsd.getHwpid();

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

      dpaRequest.SetLength(p - p0);

      TRC_FUNCTION_LEAVE("");
      return dpaRequest;
    }

    void processDpaTransactionResult(JsDriverDpaCommandSolver & jsd, std::unique_ptr<IDpaTransactionResult2> res)
    {
      TRC_FUNCTION_ENTER("");

      using namespace rapidjson;

      jsd.processResult(std::move(res));

      Document doc;

      std::string functionNameRsp(jsd.functionName());
      functionNameRsp += "_Response_rsp";

      std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;
      pnumStr = encodeHexaNum(jsd.getPnum());
      pcmdStr = encodeHexaNum(jsd.getPcmd());
      rcodeStr = encodeHexaNum(jsd.getRcode());
      dpavalStr = encodeHexaNum(jsd.getDpaval());

      //nadr, hwpid is not interesting for drivers
      Pointer("/pnum").Set(doc, pnumStr);
      Pointer("/pcmd").Set(doc, pcmdStr);
      Pointer("/rcode").Set(doc, rcodeStr);
      Pointer("/dpaval").Set(doc, rcodeStr);

      if (jsd.getRdata().size() > 0) {
        Pointer("/rdata").Set(doc, encodeBinary(jsd.getRdata().data(), jsd.getRdata().size()));
      }

      // original rawHdpRequest request passed for additional driver processing, e.g. sensor breakdown parsing
      if (jsd.storeRequest().size() > 0) {
        Document rawHdpRequestDoc;
        rawHdpRequestDoc.Parse(jsd.storeRequest());
        const Value & val = rawHdpRequestDoc;
        Pointer("/originalRequest").Set(doc, val);
      }

      std::string rawHdpResponse;
      rapidjson::StringBuffer buffer;
      rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
      doc.Accept(writer);
      rawHdpResponse = buffer.GetString();

      TRC_DEBUG(PAR(rawHdpResponse))

      try {
        std::string rsp;
        m_iJsRenderService->callFenced(jsd.getNadr(), functionNameRsp, rawHdpResponse, rsp);

        Document rspDoc;
        rspDoc.Parse(rsp);

        jsd.parseResponse(rspDoc);
      }
      catch (std::exception &e) {
        CATCH_EXC_TRC_WAR(std::exception, e, "Driver response failure: ");
        //TODO special response error exc
        THROW_EXC_TRC_WAR(std::logic_error, "Driver response failure: " << e.what());
      }

      TRC_FUNCTION_LEAVE("");
    }


  public:
    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************************" << std::endl <<
        "JsDriverService instance activate" << std::endl <<
        "******************************************"
      );

      TRC_FUNCTION_LEAVE("");
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "**************************************" << std::endl <<
        "JsDriverService instance deactivate" << std::endl <<
        "**************************************"
      );

      TRC_FUNCTION_LEAVE("");
    }

    void modify(const shape::Properties *props)
    {
    }

    void attachInterface(IJsRenderService* iface)
    {
      m_iJsRenderService = iface;
    }

    void detachInterface(IJsRenderService* iface)
    {
      if (m_iJsRenderService == iface) {
        m_iJsRenderService = nullptr;
      }
    }

  };

  JsDriverService::JsDriverService()
  {
    m_imp = shape_new Imp();
  }

  JsDriverService::~JsDriverService()
  {
    delete m_imp;
  }

  DpaMessage JsDriverService::createDpaRequest(JsDriverDpaCommandSolver & JsDriverDpaCommandSolver)
  {
    return m_imp->createDpaRequest(JsDriverDpaCommandSolver);
  }

  void JsDriverService::processDpaTransactionResult(JsDriverDpaCommandSolver & JsDriverDpaCommandSolver, std::unique_ptr<IDpaTransactionResult2> res)
  {
    return m_imp->processDpaTransactionResult(JsDriverDpaCommandSolver, std::move(res));
  }

  void JsDriverService::attachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsDriverService::detachInterface(iqrf::IJsRenderService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsDriverService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsDriverService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void JsDriverService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsDriverService::deactivate()
  {
    m_imp->deactivate();
  }

  void JsDriverService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

}