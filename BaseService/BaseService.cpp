#define IBaseService_EXPORTS

#include "DpaTransactionTask.h"
#include "BaseService.h"
#include "Trace.h"

#include "iqrf__BaseService.hxx"

//TODO workaround old tracing 
#include "IqrfLogging.h"
TRC_INIT();

TRC_INIT_MODULE(iqrf::BaseService);

namespace iqrf {
  BaseService::BaseService()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  BaseService::~BaseService()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::handleMsgFromMessaging(const std::basic_string<uint8_t> & msg)
  {
    TRC_INFORMATION(std::endl << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl <<
      "Message to process: " << std::endl << MEM_HEX_CHAR(msg.data(), msg.size()));

    //to encode output message
    std::ostringstream os;

    //get input message
    std::string msgs((const char*)msg.data(), msg.size());
    std::istringstream is(msgs);

    std::unique_ptr<DpaTask> dpaTask;
    std::string command;

    //parse
    bool handled = false;
    std::string ctype;
    std::string lastError = "Unknown ctype";
    //for (auto ser : m_serializerVect) {
      ctype = m_serializer->parseCategory(msgs);
      if (ctype == CAT_DPA_STR) {
        dpaTask = m_serializer->parseRequest(msgs);
        if (dpaTask) {
          DpaTransactionTask trans(*dpaTask);
          m_dpa->executeDpaTransaction(trans);
          int result = trans.waitFinish();
          os << dpaTask->encodeResponse(trans.getErrorStr());
          //TODO
          //just stupid hack for test async - remove it
          ///////
          //handleAsyncDpaMessage(dpaTask->getResponse());
          //handleAsyncDpaMessage(dpaTask->getRequest());
          ///////
          handled = true;
        }
        lastError = m_serializer->getLastError();
        //break;
      }
      else if (ctype == CAT_CONF_STR) {
        command = m_serializer->parseConfig(msgs);
        if (!command.empty()) {
          //TODO
          //std::string response = m_daemon->doCommand(command);
          //lastError = m_serializer->getLastError();
          //os << m_serializer->encodeConfig(msgs, response);
          handled = true;
        }
        lastError = m_serializer->getLastError();
        //break;
      }
    //}

    if (!handled) {
      os << "PARSE ERROR: " << PAR(ctype) << PAR(lastError);
    }

    std::basic_string<uint8_t> msgu((unsigned char*)os.str().data(), os.str().size());

    TRC_INFORMATION("Response to send: " << std::endl << MEM_HEX_CHAR(msgu.data(), msgu.size()) << std::endl <<
      ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl);
    
    m_messaging->sendMessage(msgu);
  }

  void BaseService::handleAsyncDpaMessage(const DpaMessage& dpaMessage)
  {
    TRC_FUNCTION_ENTER("");
    std::string sr = m_serializer->encodeAsyncAsDpaRaw(dpaMessage);
    TRC_INFORMATION(std::endl << "<<<<< ASYNCHRONOUS <<<<<<<<<<<<<<<" << std::endl <<
      "Asynchronous message to send: " << std::endl << MEM_HEX(sr.data(), sr.size()) << std::endl <<
      ">>>>> ASYNCHRONOUS >>>>>>>>>>>>>>>" << std::endl);
    std::basic_string<uint8_t> msgu((unsigned char*)sr.data(), sr.size());

    m_messaging->sendMessage(msgu);
    
    TRC_FUNCTION_LEAVE("");
  }

  void BaseService::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "BaseService instance activate" << std::endl <<
      "******************************"
    );

    //TODO workaround old tracing 
    TRC_START("TraceOldBaseService.txt", iqrf::Level::dbg, TRC_DEFAULT_FILE_MAXSIZE);

    props->getMemberAsString("instance", m_name);

    TRC_DEBUG("Attached IMessagingService: " << m_messaging->getName());

    m_scheduler->registerMessageHandler(m_name, [&](const std::string& msg) {
      std::basic_string<uint8_t> msgu((unsigned char*)msg.data(), msg.size());
      handleMsgFromMessaging(msgu);
    });

    if (m_asyncDpaMessage) {
      TRC_INFORMATION("Set AsyncDpaMessageHandler :" << PAR(m_name));
      m_dpa->registerAsyncMessageHandler(m_name, [&](const DpaMessage& dpaMessage) {
        handleAsyncDpaMessage(dpaMessage);
      });
    }

    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "******************************" << std::endl <<
      "BaseService instance deactivate" << std::endl <<
      "******************************"
    );

    m_scheduler->unregisterMessageHandler(m_name);

    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::modify(const shape::Properties *props)
  {
  }

  void BaseService::attachInterface(iqrf::IMessagingService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_messaging = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::detachInterface(iqrf::IMessagingService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_messaging == iface) {
      m_messaging = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::attachInterface(iqrf::IJsonSerializerService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_serializer = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::detachInterface(iqrf::IJsonSerializerService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_serializer == iface) {
      m_serializer = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::attachInterface(iqrfgw::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_dpa = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::detachInterface(iqrfgw::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_dpa == iface) {
      m_dpa = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::attachInterface(iqrfgw::ISchedulerService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_scheduler = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::detachInterface(iqrfgw::ISchedulerService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_scheduler == iface) {
      m_scheduler = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void BaseService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void BaseService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

}
