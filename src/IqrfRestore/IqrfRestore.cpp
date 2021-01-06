#define IIqrfRestore_EXPORTS

#include "IqrfRestore.h"
#include "Trace.h"
#include "iqrf__IqrfRestore.hxx"
#include <chrono>
#include <iostream>
#include <fstream>
#include <set>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <random>
#include <cstddef>
#include <tuple>
#include <cmath>
#include <list>
#include <vector>
#include <mutex>

TRC_INIT_MODULE(iqrf::IqrfRestore);

using namespace rapidjson;

namespace iqrf {
  // Implementation class
  class IqrfRestore::Imp
  {
  private:
    // Parent object
    IqrfRestore & m_parent;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    std::mutex m_restoreMutex;
    int m_errorCode;

  public:
    Imp(IqrfRestore& parent) : m_parent(parent)
    {
    }

    ~Imp()
    {
    }

    //---------------------------------------------------------------------
    // Check presence of Coordinator and OS peripherals on coordinator node
    //---------------------------------------------------------------------
    void checkPresentCoordAndCoordOs(void)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage perEnumRequest;
        DpaMessage::DpaPacket_t perEnumPacket;
        perEnumPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        perEnumPacket.DpaRequestPacket_t.PNUM = PNUM_ENUMERATION;
        perEnumPacket.DpaRequestPacket_t.PCMD = CMD_GET_PER_INFO;
        perEnumPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        perEnumRequest.DataToBuffer(perEnumPacket.Buffer, sizeof(TDpaIFaceHeader));
        m_exclusiveAccess->executeDpaTransactionRepeat(perEnumRequest, transResult, 1);
        TRC_DEBUG("Result from Device Exploration transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Device exploration successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, perEnumRequest.PeripheralType())
          << NAME_PAR(Node address, perEnumRequest.NodeAddress())
          << NAME_PAR(Command, (int)perEnumRequest.PeripheralCommand())
        );
        // Check Coordinator and OS peripherals
        if ((dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.EmbeddedPers[PNUM_COORDINATOR / 8] & (1 << PNUM_COORDINATOR)) != (1 << PNUM_COORDINATOR))
          THROW_EXC(std::logic_error, "Coordinator peripheral NOT found.");
        if ((dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.EnumPeripheralsAnswer.EmbeddedPers[PNUM_OS / 8] & (1 << PNUM_OS)) != (1 << PNUM_OS))
          THROW_EXC(std::logic_error, "OS peripheral NOT found.");
        m_transResults.push_back(std::move(transResult));
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        m_errorCode = transResult->getErrorCode();
        m_transResults.push_back(std::move(transResult));
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //--------------------
    // Read OS information
    //--------------------
    TPerOSRead_Response readOsInfo(const uint16_t deviceAddress)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage reaodOSInfoRequest;
        DpaMessage::DpaPacket_t readOSInfoPacket;
        readOSInfoPacket.DpaRequestPacket_t.NADR = deviceAddress;
        readOSInfoPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        readOSInfoPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
        readOSInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        reaodOSInfoRequest.DataToBuffer(readOSInfoPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(reaodOSInfoRequest, transResult, 3);
        TRC_DEBUG("Result from CMD_OS_READ as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("Device CMD_OS_READ successful!");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, reaodOSInfoRequest.PeripheralType())
          << NAME_PAR(Node address, reaodOSInfoRequest.NodeAddress())
          << NAME_PAR(Command, (int)reaodOSInfoRequest.PeripheralCommand())
        );
        m_transResults.push_back(std::move(transResult));
        TRC_FUNCTION_LEAVE("");
        return(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerOSRead_Response);
      }
      catch (std::exception& e)
      {
        m_errorCode = transResult->getErrorCode();
        m_transResults.push_back(std::move(transResult));
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------
    // Get bonded nodes
    //-----------------
    std::basic_string<uint16_t> getBondedNodes(void)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage getBondedNodesRequest;
        DpaMessage::DpaPacket_t getBondedNodesPacket;
        getBondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
        getBondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
        getBondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
        getBondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        getBondedNodesRequest.DataToBuffer(getBondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(getBondedNodesRequest, transResult, 2);
        TRC_DEBUG("Result from CMD_COORDINATOR_BONDED_DEVICES transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("GCMD_COORDINATOR_BONDED_DEVICES OK.");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, getBondedNodesRequest.PeripheralType())
          << NAME_PAR(Node address, getBondedNodesRequest.NodeAddress())
          << NAME_PAR(Command, (int)getBondedNodesRequest.PeripheralCommand())
        );
        // Get response data
        std::basic_string<uint16_t> bondedNodes;
        bondedNodes.clear();
        for (uint16_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++)
          if ((dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[nodeAddr / 8] & (1 << (nodeAddr % 8))) != 0)
            bondedNodes.push_back(nodeAddr);
        m_transResults.push_back(std::move(transResult));
        TRC_FUNCTION_LEAVE("");
        return bondedNodes;
      }
      catch (std::exception& e)
      {
        m_errorCode = transResult->getErrorCode();
        m_transResults.push_back(std::move(transResult));
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //--------------------
    // Restart coordinator
    //--------------------
    void restartDevice(const uint16_t deviceAddress)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        // Prepare DPA request
        DpaMessage restartCoordRequest;
        DpaMessage::DpaPacket_t restartCoordPacket;
        restartCoordPacket.DpaRequestPacket_t.NADR = deviceAddress;
        restartCoordPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
        restartCoordPacket.DpaRequestPacket_t.PCMD = CMD_OS_RESET;
        restartCoordPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
        restartCoordRequest.DataToBuffer(restartCoordPacket.Buffer, sizeof(TDpaIFaceHeader));
        // Execute the DPA request
        m_exclusiveAccess->executeDpaTransactionRepeat(restartCoordRequest, transResult, 2);
        TRC_DEBUG("Result from CMD_OS_RESET transaction as string:" << PAR(transResult->getErrorString()));
        DpaMessage dpaResponse = transResult->getResponse();
        TRC_INFORMATION("CMD_OS_RESET OK.");
        TRC_DEBUG(
          "DPA transaction: "
          << NAME_PAR(Peripheral type, restartCoordRequest.PeripheralType())
          << NAME_PAR(Node address, restartCoordRequest.NodeAddress())
          << NAME_PAR(Command, (int)restartCoordRequest.PeripheralCommand())
        );
        m_transResults.push_back(std::move(transResult));
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        m_errorCode = transResult->getErrorCode();
        m_transResults.push_back(std::move(transResult));
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //---------------
    // Restore device
    //---------------
    void writeBackupData(const uint16_t deviceAddress, std::basic_string<uint8_t>& networkData)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        int offset = 0;
        int restoreCycles = (int)(networkData.size() / sizeof(TPerCoordinatorNodeRestore_Request));
        do
        {
          // Prepare DPA request
          DpaMessage restoreRequest;
          DpaMessage::DpaPacket_t restorePacket;
          restorePacket.DpaRequestPacket_t.NADR = deviceAddress;
          // [C] or [N] device ?
          if (deviceAddress == COORDINATOR_ADDRESS)
          {
            // [C] device
            restorePacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
            restorePacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_RESTORE;
          }
          else
          {
            // [N] device
            restorePacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
            restorePacket.DpaRequestPacket_t.PCMD = CMD_NODE_RESTORE;
          }
          restorePacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          // Put NetworkData
          networkData.copy(restorePacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorNodeRestore_Request.NetworkData, sizeof(TPerCoordinatorNodeRestore_Request), offset);
          restoreRequest.DataToBuffer(restorePacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorNodeRestore_Request));
          // Execute the DPA request
          m_exclusiveAccess->executeDpaTransactionRepeat(restoreRequest, transResult, 3);
          TRC_DEBUG("Result from CMD_COORDINATOR_RESTORE/CMD_NODE_RESTORE transaction as string:" << PAR(transResult->getErrorString()));
          DpaMessage dpaResponse = transResult->getResponse();
          TRC_INFORMATION("Restore of device " << (int)deviceAddress << " OK. Remaining blocks: " << (restoreCycles - 1));
          //std::cout << "Restore of device " << (int)deviceAddress << " OK. Remaining blocks: " << (restoreCycles - 1) << std::endl;
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(Peripheral type, restoreRequest.PeripheralType())
            << NAME_PAR(Node address, restoreRequest.NodeAddress())
            << NAME_PAR(Command, (int)restoreRequest.PeripheralCommand())
          );
          // Add transaction
          m_transResults.push_back(std::move(transResult));
          // Next block
          offset += sizeof(TPerCoordinatorNodeRestore_Request);
        } while (--restoreCycles != 0);
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        m_errorCode = transResult->getErrorCode();
        m_transResults.push_back(std::move(transResult));
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //---------------------
    // Restore single device
    //----------------------
    void restore(const uint16_t deviceAddress, std::basic_string<uint8_t>& backupData, const bool restartCoordinator)
    {
      TRC_FUNCTION_ENTER("");
      std::lock_guard<std::mutex> lck(m_restoreMutex);
      m_errorCode = 0;

      // Todo - current version of IqrfRestore supports [C] device only
      if (deviceAddress != COORDINATOR_ADDRESS)
      {
        // Todo posunout 1010
        m_errorCode = 1003;
        THROW_EXC(std::logic_error, "Restore function of [N] device is currently not supported.");
      }

      // Check backupData length
      int len = (backupData[0x01] << 0x08) | backupData[0x00];
      // Get len[B] = block count * 49 
      len *= sizeof(TPerCoordinatorNodeRestore_Request);
      // Add length and crc8
      len += 3 * sizeof(uint8_t);
      if (len != backupData.size())
      {
        m_errorCode = 1004;
        THROW_EXC(std::logic_error, "Incorrect backupData size.");
      }
      // Check backupData CRC8
      uint8_t crc8 = 0x5f;
      for (int i = 3; i < backupData.size(); i++)
        crc8 ^= backupData[i];
      if (crc8 != backupData[0x02])
      {
        m_errorCode = 1005;
        THROW_EXC(std::logic_error, "BackupData CRC8 mismatch.");
      }
      // Remove length and CRC8
      backupData.erase(0x00, 3 * sizeof(uint8_t));

      // Get exclusive access to DPA interface
      try
      {
        m_exclusiveAccess = m_iIqrfDpaService->getExclusiveAccess();
      }
      catch (std::exception& e)
      {
        m_errorCode = 1002;
        THROW_EXC(std::logic_error, e.what());
      }
      // Restore single device
      try
      {
        m_transResults.clear();
        checkPresentCoordAndCoordOs();
        TPerOSRead_Response osInfo = readOsInfo(deviceAddress);
        writeBackupData(deviceAddress, backupData);
        // Restart [C] after backup
        if (restartCoordinator == true)
        {
          restartDevice(deviceAddress);
          // Wait for async DPA packet
          std::this_thread::sleep_for(std::chrono::milliseconds(2000));
          checkPresentCoordAndCoordOs();
        }
        // Release exclusive access
        m_exclusiveAccess.reset();
        TRC_FUNCTION_LEAVE("");
      }
      catch (std::exception& e)
      {
        // Release exclusive access
        m_exclusiveAccess.reset();
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //-----------------------------------------------------------
    // Get all DPA transactions executed during restore algorithm
    //-----------------------------------------------------------
    void getTransResults(std::list<std::unique_ptr<IDpaTransactionResult2>>& transResult)
    {
      transResult.clear();
      while (m_transResults.size() != 0)
      {
        std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator it = m_transResults.begin();
        std::unique_ptr<IDpaTransactionResult2> tr = std::move(*it);
        m_transResults.pop_front();
        transResult.push_back(std::move(tr));
      }
    }

    //---------------
    // Get error code
    //---------------
    int getErrorCode(void)
    {
      return m_errorCode;
    }

    void activate(const shape::Properties *props)
    {
      TRC_FUNCTION_ENTER("");
      TRC_INFORMATION(std::endl <<
        "************************************" << std::endl <<
        "IqrfBackup instance activate" << std::endl <<
        "************************************"
      );
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
  };

  IqrfRestore::IqrfRestore()
  {
    m_imp = shape_new Imp(*this);
  }

  IqrfRestore::~IqrfRestore()
  {
    delete m_imp;
  }

  void IqrfRestore::restore(const uint16_t deviceAddress, std::basic_string<uint8_t>& backupData, const bool restartCoordinator)
  {
    m_imp->restore(deviceAddress, backupData, restartCoordinator);
  }

  std::basic_string<uint16_t> IqrfRestore::getBondedNodes(void)
  {
    return m_imp->getBondedNodes();
  }

  void IqrfRestore::getTransResults(std::list<std::unique_ptr<IDpaTransactionResult2>>& transResult)
  {
    m_imp->getTransResults(transResult);
  }

  int IqrfRestore::getErrorCode(void)
  {
    return m_imp->getErrorCode();
  }

  void IqrfRestore::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void IqrfRestore::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void IqrfRestore::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfRestore::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void IqrfRestore::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void IqrfRestore::deactivate()
  {
    m_imp->deactivate();
  }

  void IqrfRestore::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}

