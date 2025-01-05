/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define IIqrfBackup_EXPORTS

#include "IqrfBackup.h"
#include "Trace.h"
#include "iqrf__IqrfBackup.hxx"
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

TRC_INIT_MODULE(iqrf::IqrfBackup)

using namespace rapidjson;

namespace iqrf {
  // Implementation class
  class IqrfBackup::Imp
  {
  private:
    // Parent object
    IqrfBackup & m_parent;
    IIqrfDpaService* m_iIqrfDpaService = nullptr;
    std::list<std::unique_ptr<IDpaTransactionResult2>> m_transResults;
    std::unique_ptr<IIqrfDpaService::ExclusiveAccess> m_exclusiveAccess;
    std::mutex m_backupMutex;
    int m_errorCode = 0;

  public:
    Imp(IqrfBackup& parent) : m_parent(parent)
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
        m_iIqrfDpaService->executeDpaTransactionRepeat(getBondedNodesRequest, transResult, 2);
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

    //--------------
    // Backup device
    //--------------
    std::basic_string<uint8_t> readBackupData(const uint16_t deviceAddress)
    {
      TRC_FUNCTION_ENTER("");
      std::unique_ptr<IDpaTransactionResult2> transResult;
      try
      {
        uint8_t index = 0;
        int remainingBlock;
        std::basic_string<uint8_t> backupData;
        backupData.clear();
        // Data block count Low8/High8 and crc8
        backupData.append(3, 0);
        do
        {
          // Prepare DPA request
          DpaMessage backupRequest;
          DpaMessage::DpaPacket_t backupPacket;
          backupPacket.DpaRequestPacket_t.NADR = deviceAddress;
          // [C] or [N] device ?
          if (deviceAddress == COORDINATOR_ADDRESS)
          {
            // [C] device
            backupPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
            backupPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BACKUP;
          }
          else
          {
            // [N] device
            backupPacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
            backupPacket.DpaRequestPacket_t.PCMD = CMD_NODE_BACKUP;
          }
          backupPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
          // Put Index
          backupPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorNodeBackup_Request.Index = index;
          backupRequest.DataToBuffer(backupPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorNodeBackup_Request));
          // Execute the DPA request
          m_exclusiveAccess->executeDpaTransactionRepeat(backupRequest, transResult, 3);
          DpaMessage dpaResponse = transResult->getResponse();
          remainingBlock = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorNodeBackup_Response.NetworkData[sizeof(TPerCoordinatorNodeBackup_Response) - 1 * sizeof(uint8_t)];
          TRC_DEBUG("Result from CMD_COORDINATOR_BACKUP/CMD_NODE_BACKUP transaction as string:" << PAR(transResult->getErrorString()));
          TRC_INFORMATION("Backup of device " << (int)deviceAddress << " OK. Remaining blocks: " << remainingBlock);
          //std::cout << "Backup of device " << (int)deviceAddress << " OK. Remaining blocks: " << remainingBlock << std::endl;
          TRC_DEBUG(
            "DPA transaction: "
            << NAME_PAR(Peripheral type, backupRequest.PeripheralType())
            << NAME_PAR(Node address, backupRequest.NodeAddress())
            << NAME_PAR(Command, (int)backupRequest.PeripheralCommand())
          );
          // Get backup data
          backupData.append(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorNodeBackup_Response.NetworkData, sizeof(TPerCoordinatorNodeBackup_Response));
          // Add transaction
          m_transResults.push_back(std::move(transResult));
          // Next index
          index++;
        } while (remainingBlock != 0);
        // Insert block count
        backupData[0x00] = (uint8_t)(index & 0xff);
        backupData[0x01] = (uint8_t)(index >> 0x08);
        // Insert CRC
        uint8_t crc8 = 0x5f;
        for (size_t i = 3; i < backupData.size(); i++)
          crc8 ^= backupData[i];
        backupData[0x02] = crc8;
        TRC_FUNCTION_LEAVE("");
        return backupData;
      }
      catch (std::exception& e)
      {
        m_errorCode = transResult->getErrorCode();
        m_transResults.push_back(std::move(transResult));
        THROW_EXC(std::logic_error, e.what());
      }
    }

    //---------------------
    // Backup single device
    //---------------------
    void backup(const uint16_t address, DeviceBackupData& backupData)
    {
      TRC_FUNCTION_ENTER("");
      std::lock_guard<std::mutex> lck(m_backupMutex);
      m_errorCode = 0;

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
      try
      {
        // Backup single device
        m_transResults.clear();
        checkPresentCoordAndCoordOs();
        TPerOSRead_Response osInfo = readOsInfo(address);
        std::basic_string<uint8_t> data = readBackupData(address);
        backupData = DeviceBackupData(address, true, *((uint32_t*)osInfo.MID), osInfo.DpaVersion, data);
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

    //----------------------------------------------------------
    // Get all DPA transactions executed during backup algorithm
    //----------------------------------------------------------
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
      modify(props);
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
      (void)props;
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

  IqrfBackup::IqrfBackup()
  {
    m_imp = shape_new Imp(*this);
  }

  IqrfBackup::~IqrfBackup()
  {
    delete m_imp;
  }

  void IqrfBackup::backup(const uint16_t address, DeviceBackupData& backupData)
  {
    m_imp->backup(address, backupData);
  }

  std::basic_string<uint16_t> IqrfBackup::getBondedNodes(void)
  {
    return m_imp->getBondedNodes();
  }

  void IqrfBackup::getTransResults(std::list<std::unique_ptr<IDpaTransactionResult2>>& transResult)
  {
    m_imp->getTransResults(transResult);
  }

  int IqrfBackup::getErrorCode(void)
  {
    return m_imp->getErrorCode();
  }

  void IqrfBackup::attachInterface(IIqrfDpaService* iface)
  {
    m_imp->attachInterface(iface);
  }

  void IqrfBackup::detachInterface(IIqrfDpaService* iface)
  {
    m_imp->detachInterface(iface);
  }

  void IqrfBackup::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void IqrfBackup::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }

  void IqrfBackup::activate(const shape::Properties *props)
  {
    m_imp->activate(props);
  }

  void IqrfBackup::deactivate()
  {
    m_imp->deactivate();
  }

  void IqrfBackup::modify(const shape::Properties *props)
  {
    m_imp->modify(props);
  }
}

