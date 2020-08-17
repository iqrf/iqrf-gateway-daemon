#define IBackupService_EXPORTS

#include "BackupService.h"
#include "Trace.h"
#include "ComBackup.h"
#include "iqrf__BackupService.hxx"
#include <list>
#include <cmath>
#include <thread>
#include <bitset>
#include <chrono>

TRC_INIT_MODULE(iqrf::BackupService);

using namespace rapidjson;

namespace {
  // Service general fail code - may and probably will be changed later in the future
  static const int SERVICE_ERROR = 1000;
};

namespace iqrf {
  // Implementation class
  class BackupService::Imp
  {
  private:
    // Parent object
    BackupService & m_parent;

    // Message type
    const std::string m_mTypeName_Backup = "iqmeshNetwork_Backup";
    IMessagingSplitterService* m_iMessagingSplitterService = nullptr;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    IIqrfBackup* m_iIqrfBackup = nullptr;
    const std::string* m_messagingId = nullptr;
    const IMessagingSplitterService::MsgType* m_msgType = nullptr;
    const ComBackup* m_comBackup = nullptr;

  public:
    Imp(BackupService& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

    //-------------------
    // Send backup result
    //-------------------
    void sendBackupResult(const std::string statusStr, DeviceBackupData& deviceBackupData, const double progress)
    {
      Document docBackupResult;

      // Set common parameters
      Pointer("/mType").Set(docBackupResult, m_msgType->m_type);
      Pointer("/data/msgId").Set(docBackupResult, m_comBackup->getMsgId());

      // Add progress
      int p = (int)round(progress);
      Pointer("/data/rsp/progress").Set(docBackupResult, p);

      // Add backup result
      rapidjson::Value arrayDevices(kArrayType);
      Document::AllocatorType& allocator = docBackupResult.GetAllocator();
      rapidjson::Value objDeviceBackup(kObjectType);
      // Put node address
      objDeviceBackup.AddMember("address", deviceBackupData.getAddress(), allocator);
      // Put node online status
      objDeviceBackup.AddMember("online", deviceBackupData.getOnlineStatus(), allocator);
      // Device online ?
      if (deviceBackupData.getOnlineStatus() == true)
      {
        // Put MID
        std::stringstream strMID;
        strMID << std::hex << deviceBackupData.getMID();
        objDeviceBackup.AddMember("MID", strMID.str(), allocator);
        // Put DPA version
        std::ostringstream strDpaVer;
        strDpaVer << std::hex << std::setw(1) << ((deviceBackupData.getDpaVersion() & 0x3fff) >> 8) << '.' << std::setw(2) << (deviceBackupData.getDpaVersion() & 0xff);
        objDeviceBackup.AddMember("version", strDpaVer.str(), allocator);
        // Put backup data
        std::ostringstream strBackupData;
        std::basic_string<uint8_t> data = deviceBackupData.getBackupData();
        for (const uint8_t byte : data)
          strBackupData << std::setfill('0') << std::setw(2) << std::hex << (int)byte;
        objDeviceBackup.AddMember("data", strBackupData.str(), allocator);
      }
      // Put device
      arrayDevices.PushBack(objDeviceBackup, allocator);
      Pointer("/data/rsp/devices").Set(docBackupResult, arrayDevices);

      // Set raw fields, if verbose mode is active
      if (m_comBackup->getVerbose() == true)
      {
        rapidjson::Value rawArray(kArrayType);
        Document::AllocatorType& allocator = docBackupResult.GetAllocator();
        std::list<std::unique_ptr<IDpaTransactionResult2>> transResults;
        m_iIqrfBackup->getTransResults(transResults);
        while(transResults.size() != 0)
        {
          std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator iter = transResults.begin();
          std::unique_ptr<IDpaTransactionResult2> tr = std::move(*iter);
          transResults.pop_front();

          rapidjson::Value rawObject(kObjectType);

          rawObject.AddMember(
            "request",
            encodeBinary(tr->getRequest().DpaPacket().Buffer, tr->getRequest().GetLength()),
            allocator
          );

          rawObject.AddMember(
            "requestTs",
            encodeTimestamp(tr->getRequestTs()),
            allocator
          );

          rawObject.AddMember(
            "confirmation",
            encodeBinary(tr->getConfirmation().DpaPacket().Buffer, tr->getConfirmation().GetLength()),
            allocator
          );

          rawObject.AddMember(
            "confirmationTs",
            encodeTimestamp(tr->getConfirmationTs()),
            allocator
          );

          rawObject.AddMember(
            "response",
            encodeBinary(tr->getResponse().DpaPacket().Buffer, tr->getResponse().GetLength()),
            allocator
          );

          rawObject.AddMember(
            "responseTs",
            encodeTimestamp(tr->getResponseTs()),
            allocator
          );

          // add object into array
          rawArray.PushBack(rawObject, allocator);
        }

        // Add array into response document
        Pointer("/data/raw").Set(docBackupResult, rawArray);
      }

      // Set status
      int statusCode = 0;
      if (statusStr != "ok")
        statusCode = SERVICE_ERROR;
      Pointer("/data/status").Set(docBackupResult, statusCode);
      Pointer("/data/statusStr").Set(docBackupResult, statusStr);

      // Send message      
      m_iMessagingSplitterService->sendMessage(*m_messagingId, std::move(docBackupResult));
    }

    //-----------
    // Run backup
    //-----------
    void runBackup(const bool wholeNetwork, const uint16_t deviceAddress)
    {
      TRC_FUNCTION_ENTER("");

      // Network devices
      std::basic_string<uint16_t> devices;
      devices.clear();
      if (wholeNetwork == true)
      {
        // Yes, add [C] device
        devices.push_back(COORDINATOR_ADDRESS);
        std::basic_string<uint16_t> bondedNodes = m_iIqrfBackup->getBondedNodes();
        // Add all [N] devices
        devices.append(bondedNodes);
      }
      else
      {
        // No, add single device
        devices.push_back(deviceAddress);
      }

      // Backup selected devices
      double progress = 0;
      for (uint16_t device : devices)
      {
        DeviceBackupData deviceBackupData(device);
        std::string statusStr = "ok";
        try
        {
          // Backup device
          m_iIqrfBackup->backup(device, deviceBackupData);
        }
        catch (std::exception& e)
        {
          statusStr = e.what();
          CATCH_EXC_TRC_WAR(std::exception, e, "Backup device [" << (int)device << "] error.");
        }
        progress += (100.0 / devices.size());
        sendBackupResult(statusStr, deviceBackupData, progress);
      }
      TRC_FUNCTION_LEAVE("");
    }

    // Process request
    void handleMsg(const std::string& messagingId, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc)
    {
      TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) << NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));

      // Unsupported type of request
      if (msgType.m_type != m_mTypeName_Backup)
        THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));

      // Create representation object
      ComBackup comBackup(doc);

      // Parsing and checking service parameters
      TBackupInputParams backupInputParams;
      try
      {
        backupInputParams = comBackup.getBackupParams();
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Backup algorithm error.")
        // Create error response
        Document response;
        Pointer("/mType").Set(response, msgType.m_type);
        Pointer("/data/msgId").Set(response, comBackup.getMsgId());
        // Set result
        Pointer("/data/status").Set(response, SERVICE_ERROR);
        Pointer("/data/statusStr").Set(response, e.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
        TRC_FUNCTION_LEAVE("");
        return;
      }

      // Run the Backup
      m_msgType = &msgType;
      m_messagingId = &messagingId;
      m_comBackup = &comBackup;
      try
      {
        runBackup(backupInputParams.wholeNetwork, backupInputParams.deviceAddress);
      }
      catch (std::exception& e)
      {
        CATCH_EXC_TRC_WAR(std::exception, e, "Backup algorithm error.")
        // Create error response
        Document response;
        Pointer("/mType").Set(response, msgType.m_type);
        Pointer("/data/msgId").Set(response, comBackup.getMsgId());
        // Set result
        Pointer("/data/status").Set(response, SERVICE_ERROR);
        Pointer("/data/statusStr").Set(response, e.what());
        m_iMessagingSplitterService->sendMessage(messagingId, std::move(response));
      }
      TRC_FUNCTION_LEAVE("");
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "Backup instance activate" << std::endl <<
        "************************************"
      );

      // for the sake of register function parameters 
      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_Backup
      };

      m_iMessagingSplitterService->registerFilteredMsgHandler(
        supportedMsgTypes,
        [&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc)
        {
          handleMsg(messagingId, msgType, std::move(doc));
        });

      TRC_FUNCTION_LEAVE("")
    }

    void deactivate()
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "Bqackup instance deactivate" << std::endl <<
        "************************************"
      );

      std::vector<std::string> supportedMsgTypes =
      {
        m_mTypeName_Backup
      };

      m_iMessagingSplitterService->unregisterFilteredMsgHandler(supportedMsgTypes);

      TRC_FUNCTION_LEAVE("");
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

    void attachInterface(IIqrfBackup* iface)
    {
      m_iIqrfBackup = iface;
    }

    void detachInterface(IIqrfBackup* iface)
    {
      if (iface == iface)
        m_iIqrfBackup = nullptr;
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

  BackupService::BackupService()
  {
    m_imp = shape_new Imp(*this);
  }

  BackupService::~BackupService()
  {
    delete m_imp;
  }

  void BackupService::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void BackupService::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void BackupService::attachInterface(IMessagingSplitterService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void BackupService::detachInterface(IMessagingSplitterService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void BackupService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void BackupService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void BackupService::attachInterface(IIqrfBackup* iface)
  {
    m_imp->attachInterface(iface);
  }

  void BackupService::detachInterface(IIqrfBackup* iface)
  {
    m_imp->detachInterface(iface);
  }

  void BackupService::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void BackupService::deactivate()
  {
    m_imp->deactivate();
  }

  void BackupService::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}

