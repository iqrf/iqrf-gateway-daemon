#define IMessagingSplitterService_EXPORTS

#include "Raws.h"
#include "DpaHandler2.h"
#include "JsonDpaApi.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/schema.h"
#include "ITemplateService.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>

#include "iqrf__JsonDpaApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApi);

using namespace rapidjson;

namespace iqrf {
  class JsonDpaApi::Imp
  {
  private:
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::list<std::string> m_supportedMsgTypes = { "comRaw", "comRawHdp" };
    std::map<std::string, SchemaDocument> m_validatorMap;

  public:
    Imp()
    {
    }

    ~Imp()
    {
    }

    void createValidator(const std::string& msgType, const std::string& fname)
    {
      Document sd;

      std::ifstream ifs(fname);
      if (!ifs.is_open()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot open: " << PAR(fname));
      }

      rapidjson::IStreamWrapper isw(ifs);
      sd.ParseStream(isw);

      if (sd.HasParseError()) {
        THROW_EXC_TRC_WAR(std::logic_error, "Json parse error: " << NAME_PAR(emsg, sd.GetParseError()) <<
          NAME_PAR(eoffset, sd.GetErrorOffset()));
      }

      SchemaDocument schema(sd);
      m_validatorMap.insert(std::make_pair(msgType, std::move(schema)));
    }

    void validate(const std::string& msgType, const Document& doc)
    {
      auto found = m_validatorMap.find(msgType);
      if (found != m_validatorMap.end()) {
        SchemaValidator validator(found->second);
        if (!doc.Accept(validator)) {
          // Input JSON is invalid according to the schema
          // Output diagnostic information
          StringBuffer sb;
          validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
          printf("Invalid schema: %s\n", sb.GetString());
          printf("Invalid keyword: %s\n", validator.GetInvalidSchemaKeyword());
          sb.Clear();
          validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
          printf("Invalid document: %s\n", sb.GetString());
          THROW_EXC_TRC_WAR(std::logic_error, "Invalid");
        }
        TRC_DEBUG("OK");
      }
      else {
        //TODO why
        THROW_EXC_TRC_WAR(std::logic_error, "Cannot find validator");
      }
    }

    void handleMsg(const std::string & messagingId, const std::string & msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << PAR(msgType));
      validate(msgType, doc);
      Document respDoc;
      std::unique_ptr<ComBase> com;

      if (msgType == "comRaw") {
        com.reset(shape_new ComRaw(doc));
      }
      else if (msgType == "comRawHdp") {
        com.reset(shape_new ComRawHdp(doc));
      }

      auto trn = m_iIqrfDpaService->executeDpaTransaction(com->getDpaRequest());
      auto res = trn->get();
      com->createResponse(respDoc, *res);

      //update message type - type is the same for request/response
      Pointer("/mType").Set(respDoc, msgType);

      //TODO validate response in debug
      m_iMessagingSplitterService->sendMessage(messagingId, std::move(respDoc));

      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApi instance activate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->registerFilteredMsgHandler(m_supportedMsgTypes,
        [&](const std::string & messagingId, const std::string & msgType, rapidjson::Document doc)
      {
        handleMsg(messagingId, msgType, std::move(doc));
      });

      ////////////////////////////
      createValidator("comRaw", "./configuration/JsonSchemes/async-api-json-schemes/comRaw-request.json");
      createValidator("comRawHdp", "./configuration/JsonSchemes/async-api-json-schemes/comRawHdp-request.json");
      ////////////////////////////

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "******************************" << std::endl <<
        "JsonDpaApi instance deactivate" << std::endl <<
        "******************************"
      );

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_supportedMsgTypes);

      TRC_FUNCTION_LEAVE("")
    }

    void modify(const shape::Properties *props)
    {
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
  JsonDpaApi::JsonDpaApi()
  {
    m_imp = shape_new Imp();
  }

  JsonDpaApi::~JsonDpaApi()
  {
    delete m_imp;
  }

  void JsonDpaApi::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void JsonDpaApi::deactivate()
  {
    m_imp->deactivate();
  }

  void JsonDpaApi::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }

  void JsonDpaApi::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApi::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApi::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void JsonDpaApi::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void JsonDpaApi::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void JsonDpaApi::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
