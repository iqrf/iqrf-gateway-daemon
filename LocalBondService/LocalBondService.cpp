#define ILocalBondService_EXPORTS

#include "DpaTransactionTask.h"
#include "LocalBondService.h"
#include "Trace.h"
#include "PrfOs.h"
#include "DpaRaw.h"
#include "PrfFrc.h"

#include "iqrf__LocalBondService.hxx"

//TODO workaround old tracing 
#include "IqrfLogging.h"


TRC_INIT();

TRC_INIT_MODULE(iqrf::LocalBondService);

namespace {

  // Default bonding mask. No masking effect.
  static const uint8_t DEFAULT_BONDING_MASK = 0;
};


namespace iqrf {

  LocalBondService::LocalBondService()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  LocalBondService::~LocalBondService()
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  
  void LocalBondService::attachInterface(iqrf::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    m_dpa = iface;
    TRC_FUNCTION_LEAVE("")
  }

  void LocalBondService::detachInterface(iqrf::IIqrfDpaService* iface)
  {
    TRC_FUNCTION_ENTER(PAR(iface));
    if (m_dpa == iface) {
      m_dpa = nullptr;
    }
    TRC_FUNCTION_LEAVE("")
  }

  void LocalBondService::attachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().addTracerService(iface);
  }

  void LocalBondService::detachInterface(shape::ITraceService* iface)
  {
    shape::Tracer::get().removeTracerService(iface);
  }


  void LocalBondService::activate(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "**********************************" << std::endl <<
      "LocalBondService instance activate" << std::endl <<
      "**********************************"
    );

    //TODO workaround old tracing 
    TRC_START("TraceOldBaseService.txt", iqrf::Level::dbg, TRC_DEFAULT_FILE_MAXSIZE);
    TRC_FUNCTION_LEAVE("")
  }

  void LocalBondService::deactivate()
  {
    TRC_FUNCTION_ENTER("");
    TRC_INFORMATION(std::endl <<
      "************************************" << std::endl <<
      "LocalBondService instance deactivate" << std::endl <<
      "************************************"
    );

    TRC_FUNCTION_LEAVE("");
  }

  void LocalBondService::modify(const shape::Properties *props)
  {
    TRC_FUNCTION_ENTER("");
    TRC_FUNCTION_LEAVE("");
  }

  void LocalBondService::checkNodeAddr(const uint16_t nodeAddr)
  {
    if ((nodeAddr < 0) || (nodeAddr > 0xEF)) {
      THROW_EXC(
        std::exception, "Node address outside of valid range. " << NAME_PAR_HEX("Address", nodeAddr)
      );
    }
  }

  // trys to bonds node and returns result
  BondResult LocalBondService::_bondNode(const uint16_t nodeAddr) {
    TRC_FUNCTION_ENTER("");

    DpaMessage bondNodeRequest;
    DpaMessage::DpaPacket_t bondNodePacket;
    bondNodePacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
    bondNodePacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
    bondNodePacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BOND_NODE;
    bondNodePacket.DpaRequestPacket_t.HWPID = HWPID_Default;

    TPerCoordinatorBondNode_Request* tCoordBondNodeRequest = &bondNodeRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerCoordinatorBondNode_Request;
    tCoordBondNodeRequest->ReqAddr = uint8_t(nodeAddr & 0xFF);
    tCoordBondNodeRequest->BondingMask = DEFAULT_BONDING_MASK;

    bondNodeRequest.DataToBuffer(bondNodePacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

    // issue the DPA request
    DpaRaw bondNodeTask(bondNodeRequest);
    DpaTransactionTask bondNodeTransaction(bondNodeTask);

    try {
      m_dpa->executeDpaTransaction(bondNodeTransaction);
    }
    catch (std::exception& e) {
      bondNodeTransaction.processFinish(DpaTransfer::kError);
      TRC_DEBUG("DPA transaction error : " << e.what());
      THROW_EXC(std::exception, "Could not bond node.");
    }

    int transResult = bondNodeTransaction.waitFinish();

    TRC_DEBUG("Result from bond node transaction as string:" << PAR(bondNodeTransaction.getErrorStr()));

    if (transResult == 0) {
      TRC_INFORMATION("Bond node done!");
      TRC_DEBUG("DPA transaction: " << NAME_PAR(bondNodeTask.getPrfName(), bondNodeTask.getAddress()) << PAR(bondNodeTask.encodeCommand()));

      // getting response data
      DpaMessage dpaResponse = bondNodeTask.getResponse();

      // check response code
      uint8_t respCode = dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode;
      if (respCode == STATUS_NO_ERROR) {
        TRC_INFORMATION("Bond node successful.");
      }
      else {
        TRC_DEBUG("Bond node NOT successful. " << NAME_PAR_HEX("Response code", respCode));
        THROW_EXC(std::exception, "Could not bond a node.");
      }

      // getting bond data
      TPerCoordinatorBondNode_Response bondNodeResponse
        = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorBondNode_Response;

      TRC_FUNCTION_LEAVE("");
      return BondResult(BondResult::Type::NoError, bondNodeResponse.BondAddr, bondNodeResponse.DevNr);
    }
    
    TRC_DEBUG("DPA transaction error. " << NAME_PAR_HEX("Result", transResult));
    THROW_EXC(std::exception, "Could not bond a node.");
  }

  // pings specified node using OS::Read command
  bool LocalBondService::pingNode(const uint16_t nodeAddr) 
  {
    TRC_FUNCTION_ENTER("");

    DpaMessage readInfoRequest;
    DpaMessage::DpaPacket_t readInfoPacket;
    readInfoPacket.DpaRequestPacket_t.NADR = nodeAddr;
    readInfoPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
    readInfoPacket.DpaRequestPacket_t.PCMD = CMD_OS_READ;
    readInfoPacket.DpaRequestPacket_t.HWPID = HWPID_Default;

    readInfoRequest.DataToBuffer(readInfoPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

    // issue the DPA request
    DpaRaw readInfoTask(readInfoRequest);
    DpaTransactionTask readInfoTransaction(readInfoTask);

    try {
      m_dpa->executeDpaTransaction(readInfoTransaction);
    }
    catch (std::exception& e) {
      readInfoTransaction.processFinish(DpaTransfer::kError);
      TRC_DEBUG("DPA transaction error : " << e.what());
      THROW_EXC(std::exception, "Could not read node's info.");
    }

    int transResult = readInfoTransaction.waitFinish();

    TRC_DEBUG("Result from read node's info transaction as string:" << PAR(readInfoTransaction.getErrorStr()));

    if (transResult == 0) {
      TRC_INFORMATION("Read node's info done!");
      TRC_DEBUG("DPA transaction :" << NAME_PAR(readInfoTask.getPrfName(), readInfoTask.getAddress()) << PAR(readInfoTask.encodeCommand()));

      // getting response data
      DpaMessage dpaResponse = readInfoTask.getResponse();

      // check response code
      uint8_t respCode = dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode;
      if (respCode == STATUS_NO_ERROR) {
        TRC_INFORMATION("Read node's info successful!");
        TRC_FUNCTION_LEAVE("");
        return true;
      }
      else {
        TRC_DEBUG("Read node's info NOT successful. " << NAME_PAR_HEX("Response code", respCode));
        return false;
      }
    }

    TRC_DEBUG("DPA transaction error. " << NAME_PAR_HEX("Result", transResult));
    THROW_EXC(std::exception, "Could not read node's info.");
  }

  // removes specified address from coordinator's list of bonded addresses
  void LocalBondService::removeBondedNode(const uint16_t nodeAddr) {
    TRC_FUNCTION_ENTER("");

    DpaMessage removeBondRequest;
    DpaMessage::DpaPacket_t removeBondPacket;
    removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
    removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
    removeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
    removeBondPacket.DpaRequestPacket_t.HWPID = HWPID_Default;

    TPerCoordinatorRemoveRebondBond_Request* tCoordRemoveBondRequest 
      = &removeBondRequest.DpaPacket().DpaRequestPacket_t.DpaMessage.PerCoordinatorRemoveRebondBond_Request;
    tCoordRemoveBondRequest->BondAddr = uint8_t(nodeAddr & 0xFF);

    removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + 2);

    // issue the DPA request
    DpaRaw removeBondTask(removeBondRequest);
    DpaTransactionTask removeBondTransaction(removeBondTask);

    try {
      m_dpa->executeDpaTransaction(removeBondTransaction);
    }
    catch (std::exception& e) {
      removeBondTransaction.processFinish(DpaTransfer::kError);
      TRC_DEBUG("DPA transaction error : " << e.what());
      THROW_EXC(std::exception, "Could not remove bond.");
    }

    int transResult = removeBondTransaction.waitFinish();

    TRC_DEBUG("Result from remove bond transaction as string:" << PAR(removeBondTransaction.getErrorStr()));

    if (transResult == 0) {
      TRC_INFORMATION("Remove bond done!");
      TRC_DEBUG("DPA transaction :" << NAME_PAR(removeBondTask.getPrfName(), removeBondTask.getAddress()) << PAR(removeBondTask.encodeCommand()));

      // getting response data
      DpaMessage dpaResponse = removeBondTask.getResponse();

      // check response code
      uint8_t respCode = dpaResponse.DpaPacket().DpaResponsePacket_t.ResponseCode;
      if (respCode == STATUS_NO_ERROR) {
        TRC_INFORMATION("Remove bond successful!");
      } else {
        TRC_DEBUG("Remove bond NOT successful. " << NAME_PAR_HEX("Response code", respCode));
        THROW_EXC(std::exception, "Could not remove bond.");
      }
    }
    else {
      TRC_DEBUG("DPA transaction error. " << NAME_PAR_HEX("Result", transResult));
      THROW_EXC(std::exception, "Could not remove bond.");
    }

    TRC_FUNCTION_LEAVE("");
  }

  BondResult LocalBondService::bondNode(const uint16_t nodeAddr)
  {
    // basic check
    checkNodeAddr(nodeAddr);
    
    // bond node
    BondResult bondResult = _bondNode(nodeAddr);

    // bonding node failed
    if (bondResult.getType() != BondResult::Type::NoError) {
      return bondResult;
    }

    // ping newly bonded node
    bool pingResult = pingNode(bondResult.getBondedAddr());
    if (pingResult) {
      return bondResult;
    }

    // if ping failed, remove bonded node from the coordinator's list
    removeBondedNode(bondResult.getBondedAddr());

    // and return Ping Failed error
    return BondResult(BondResult::Type::PingFailed, 0, 0);
  }
  
}