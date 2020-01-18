#define IMessagingSplitterService_EXPORTS

#include "JsonDpaApiRaw.h"
#include "ComRaws.h"
#include "ObjectFactory.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "ITemplateService.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>

#include "iqrf__JsonDpaApiRaw.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApiRaw);

using namespace rapidjson;

namespace iqrf {
  class FakeAsyncTransactionResult : public IDpaTransactionResult2
  {
  public:
    FakeAsyncTransactionResult(const DpaMessage& dpaMessage)
      :m_now(std::chrono::system_clock::now())
    {
      switch (dpaMessage.MessageDirection()) {
      case DpaMessage::MessageType::kRequest:
        m_request = dpaMessage;
      case DpaMessage::MessageType::kResponse:
        m_response = dpaMessage;
      default:;
      }
    }

    int getErrorCode() const override { return STATUS_NO_ERROR; }
    void overrideErrorCode(ErrorCode err) override {
      (void)err; //silence -Wunused-parameter
    }
    std::string getErrorString() const override { return "ok"; }

    virtual const DpaMessage& getRequest() const override { return m_request; }
    virtual const DpaMessage& getConfirmation() const override { return m_confirmation; }
    virtual const DpaMessage& getResponse() const override { return m_response; }
    virtual const std::chrono::time_point<std::chrono::system_clock>& getRequestTs() const override { return m_now; }
    virtual const std::chrono::time_point<std::chrono::system_clock>& getConfirmationTs() const override { return m_now; }
    virtual const std::chrono::time_point<std::chrono::system_clock>& getResponseTs() const override { return m_now; }
    virtual bool isConfirmed() const override { return false; }
    virtual bool isResponded() const override { return false; }
    virtual ~FakeAsyncTransactionResult() {};
  private:
    std::chrono::time_point<std::chrono::system_clock> m_now;
    DpaMessage m_confirmation;
    DpaMessage m_request;
    DpaMessage m_response;
  };

  class JsonDpaApiRaw::Imp
  {
  private:
    IMetaDataApi* m_iMetaDataApi = nullptr;
    IIqrfInfo* m_iIqrfInfo = nullptr;
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::string m_name = "JsonDpaApiRaw";
    bool m_asyncDpaMessage = false;

    // TODO from cfg
    std::vector<std::string> m_filters =
    {
      "iqrfRaw",
      "iqrfRawHdp",
    };

    ObjectFactory<ComNadr, rapidjson::Document&> m_objectFactory;

  public:
    Imp()
    {
      const std::string mType_iqrfRaw = "iqrfRaw";
      const std::string mType_iqrfRawHdp = "iqrfRawHdp";
      m_objectFactory.registerClass<ComRaw>(mType_iqrfRaw);
      m_objectFactory.registerClass<ComRawHdp>(mType_iqrfRawHdp);
    }

    ~Imp()
    {
    }

    void handleMsg(const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
        NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));


      std::unique_ptr<ComNadr> com = m_objectFactory.createObject(msgType.m_type, doc);

      // old metadata
      if (m_iMetaDataApi) {
        if (m_iMetaDataApi->iSmetaDataToMessages()) {
          com->setMetaData(m_iMetaDataApi->getMetaData(com->getNadr()));
        }
      }

      // db metadata
      if (m_iIqrfInfo) {
        if (m_iIqrfInfo->getMidMetaDataToMessages()) {
          com->setMidMetaData(m_iIqrfInfo->getNodeMetaData(com->getNadr()));
        }
      }

      auto trn = m_iIqrfDpaService->executeDpaTransaction(com->getDpaRequest(), com->getTimeout());
      auto res = trn->get();

      Document respDoc;
      com->setStatus(res->getErrorString(), res->getErrorCode());
      com->createResponse(respDoc, *res);

      //update message type - type is the same for request/response
      Pointer("/mType").Set(respDoc, msgType.m_type);

      //TODO validate response in debug
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void handleAsyncDpaMessage(const DpaMessage& msg)
    {
      using namespace rapidjson;

      Document fakeRequest;
      Pointer("/mType").Set(fakeRequest, "iqrfRaw");
      Pointer("/data/msgId").Set(fakeRequest, "async");
      std::string rData = encodeBinary(msg.DpaPacket().Buffer, msg.GetLength());
      //Pointer("/data/req/rData").Set(fakeRequest, "00.00.00.00.00.00");
      Pointer("/data/req/rData").Set(fakeRequest, rData);
      Document respDoc;

      ComRaw asyncResp(fakeRequest);
      FakeAsyncTransactionResult res(msg);

      asyncResp.setStatus(res.getErrorString(), res.getErrorCode());
      asyncResp.createResponse(respDoc, res);

      //update message type - type is the same for request/response
      Pointer("/mType").Set(respDoc, "iqrfRaw");

      m_iMessagingSplitterService->sendMessage("", std::move(respDoc));
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApiRaw instance activate" << std::endl <<
        "******************************"
      );

      const Document& doc = props->getAsJson();

      const Value* v = Pointer("/instance").Get(doc);
      if (v && v->IsString()) {
        m_name = v->GetString();
      }
      v = Pointer("/asyncDpaMessage").Get(doc);
      if (v && v->IsBool()) {
        m_asyncDpaMessage = v->GetBool();
      }

      m_iMessagingSplitterService->registerFilteredMsgHandler(m_filters,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
      {
        handleMsg(messagingId, msgType, std::move(doc));
      });

      if (m_asyncDpaMessage) {
        m_iIqrfDpaService->registerAsyncMessageHandler(m_name, [&](const DpaMessage& dpaMessage) {
          handleAsyncDpaMessage(dpaMessage);
        });
      }

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApiRaw instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);
      m_iIqrfDpaService->unregisterAsyncMessageHandler(m_name);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
      (void)props; //silence -Wunused-parameter
    }

    void attachInterface(IMetaDataApi* iface)
    {
      m_iMetaDataApi = iface;
    }

    void detachInterface(IMetaDataApi* iface)
    {
      if (m_iMetaDataApi == iface) {
        m_iMetaDataApi = nullptr;
      }
    }

    void attachInterface(IIqrfInfo* iface)
    {
      m_iIqrfInfo = iface;
    }

    void detachInterface(IIqrfInfo* iface)
    {
      if (m_iIqrfInfo == iface) {
        m_iIqrfInfo = nullptr;
      }
    }

    void attachInterface(IIqrfDpaService* iface)
    {
      m_iIqrfDpaService = iface;
    }

    void detachInterface(IIqrfDpaService* iface)
    {
      if (m_iIqrfDpaService == iface) {
        m_iIqrfDpaService = nullptr;
      }
    }

    void attachInterface(IMessagingSplitterService* iface)
    {
      m_iMessagingSplitterService = iface;
    }

    void detachInterface(IMessagingSplitterService* iface)
    {
      if (m_iMessagingSplitterService == iface) {
        m_iMessagingSplitterService = nullptr;
      }

    }

  };

  /////////////////////////
  JsonDpaApiRaw::JsonDpaApiRaw()
  {
    m_imp = shape_new Imp();
  }

  JsonDpaApiRaw::~JsonDpaApiRaw()
  {
    delete m_imp;
  }

  void JsonDpaApiRaw::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonDpaApiRaw::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonDpaApiRaw::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonDpaApiRaw::attachInterface(IMetaDataApi* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiRaw::detachInterface(IMetaDataApi* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiRaw::attachInterface(IIqrfInfo* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiRaw::detachInterface(IIqrfInfo* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiRaw::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiRaw::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiRaw::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApiRaw::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApiRaw::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonDpaApiRaw::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
