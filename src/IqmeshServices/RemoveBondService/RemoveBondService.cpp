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

#define IRemoveBondService_EXPORTS

#include "RemoveBondService.h"
#include "iqrf__RemoveBondService.hxx"

TRC_INIT_MODULE(iqrf::RemoveBondService)

using namespace rapidjson;

namespace iqrf {

	RemoveBondService::RemoveBondService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	RemoveBondService::~RemoveBondService() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Message handling

	void RemoveBondService::handleMsg(const std::string& messagingId, const IMessagingSplitterService::MsgType& msgType, rapidjson::Document doc) {
		TRC_FUNCTION_ENTER(
			PAR(messagingId) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(micro, msgType.m_micro)
		);
		// Unsupported type of request
		if ((msgType.m_type != m_mTypeName_iqmeshNetworkRemoveBond)) {
			THROW_EXC(std::logic_error, "Unsupported message type: " << PAR(msgType.m_type));
		}
		// Creating representation object
		ComIqmeshNetworkRemoveBond comRemoveBond(doc);
		m_msgType = &msgType;
		m_messagingId = &messagingId;
		m_comRemoveBond = &comRemoveBond;
		// Parsing and checking service parameters
		try {
			m_requestParams = comRemoveBond.getRequestParameters();
		} catch (const std::exception& e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "Error while parsing service input parameters.");
			createResponse(ErrorCodes::requestParseError, e.what());
			TRC_FUNCTION_LEAVE("");
			return;
		}
		// Try to establish exclusive access
		try {
			m_exclusiveAccess = m_dpaService->getExclusiveAccess();
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "Exclusive access error.");
			createResponse(ErrorCodes::exclusiveAccessError, e.what());
			TRC_FUNCTION_LEAVE("");
			return;
		}
		try {
			// Remove bond result
			RemoveBondResult removeBondResult;
			if (m_requestParams.coordinatorOnly) {
				removeBondOnlyInC(removeBondResult);
			} else {
				removeBond(removeBondResult);
			}
			// Create and send response
			createResponse(removeBondResult);
		} catch (const std::exception& e) {
			m_exclusiveAccess.reset();
			THROW_EXC_TRC_WAR(std::logic_error, e.what());
		}

		// Release exclusive access
		m_exclusiveAccess.reset();
		TRC_FUNCTION_LEAVE("");
	}

	std::set<uint8_t> RemoveBondService::getBondedNodes(RemoveBondResult& removeBondResult) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			// Prepare DPA request
			DpaMessage getBondedNodesRequest;
			DpaMessage::DpaPacket_t getBondedNodesPacket;
			getBondedNodesPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			getBondedNodesPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			getBondedNodesPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_BONDED_DEVICES;
			getBondedNodesPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			getBondedNodesRequest.DataToBuffer(getBondedNodesPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute the DPA request
			m_exclusiveAccess->executeDpaTransactionRepeat(getBondedNodesRequest, transResult, m_requestParams.repeat);
			TRC_DEBUG("Result from CMD_COORDINATOR_BONDED_DEVICES transaction as string:" << PAR(transResult->getErrorString()));
			DpaMessage dpaResponse = transResult->getResponse();
			TRC_INFORMATION("CMD_COORDINATOR_BONDED_DEVICES OK.");
			TRC_DEBUG(
				"DPA transaction: "
				<< NAME_PAR(Peripheral type, getBondedNodesRequest.PeripheralType())
				<< NAME_PAR(Node address, getBondedNodesRequest.NodeAddress())
				<< NAME_PAR(Command, (int)getBondedNodesRequest.PeripheralCommand())
			);
			// Get response data
			std::set<uint8_t> bondedNodes;
			bondedNodes.clear();
			for (uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++) {
				if ((dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.Response.PData[nodeAddr / 8] & (1 << (nodeAddr % 8))) != 0) {
					bondedNodes.insert(nodeAddr);
				}
			}
			removeBondResult.addTransactionResult(transResult);
			TRC_FUNCTION_LEAVE("");
			return bondedNodes;
		} catch (const std::exception& e) {
			removeBondResult.setStatus(transResult->getErrorCode(), e.what());
			removeBondResult.addTransactionResult(transResult);
			THROW_EXC(std::logic_error, e.what());
		}
	}

	TPerCoordinatorAddrInfo_Response RemoveBondService::getAddressingInfo(RemoveBondResult& removeBondResult) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			// Prepare DPA request
			DpaMessage addrInfoRequest;
			DpaMessage::DpaPacket_t addrInfoPacket;
			addrInfoPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			addrInfoPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			addrInfoPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_ADDR_INFO;
			addrInfoPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			addrInfoRequest.DataToBuffer(addrInfoPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute the DPA request
			m_exclusiveAccess->executeDpaTransactionRepeat(addrInfoRequest, transResult, m_requestParams.repeat);
			TRC_DEBUG("Result from Get addressing information transaction as string:" << PAR(transResult->getErrorString()));
			DpaMessage dpaResponse = transResult->getResponse();
			TRC_INFORMATION("Get addressing information successful!");
			TRC_DEBUG(
				"DPA transaction: "
				<< NAME_PAR(Peripheral type, addrInfoRequest.PeripheralType())
				<< NAME_PAR(Node address, addrInfoRequest.NodeAddress())
				<< NAME_PAR(Command, (int)addrInfoRequest.PeripheralCommand())
			);
			removeBondResult.addTransactionResult(transResult);
			removeBondResult.setNodesNr(dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAddrInfo_Response.DevNr);
			TRC_FUNCTION_LEAVE("");
			return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerCoordinatorAddrInfo_Response;
		} catch (const std::exception& e) {
			removeBondResult.setStatus(transResult->getErrorCode(), e.what());
			removeBondResult.addTransactionResult(transResult);
			THROW_EXC(std::logic_error, e.what());
		}
	}

	uint8_t RemoveBondService::setFrcReponseTime(RemoveBondResult& removeBondResult, uint8_t FRCresponseTime) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			// Prepare DPA request
			DpaMessage setFrcParamRequest;
			DpaMessage::DpaPacket_t setFrcParamPacket;
			setFrcParamPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			setFrcParamPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			setFrcParamPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SET_PARAMS;
			setFrcParamPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			setFrcParamPacket.DpaRequestPacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams = FRCresponseTime;
			setFrcParamRequest.DataToBuffer(setFrcParamPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerFrcSetParams_RequestResponse));
			// Execute the DPA request
			m_exclusiveAccess->executeDpaTransactionRepeat(setFrcParamRequest, transResult, m_requestParams.repeat);
			TRC_DEBUG("Result from CMD_FRC_SET_PARAMS transaction as string:" << PAR(transResult->getErrorString()));
			DpaMessage dpaResponse = transResult->getResponse();
			TRC_INFORMATION("CMD_FRC_SET_PARAMS OK.");
			TRC_DEBUG(
				"DPA transaction: "
				<< NAME_PAR(Peripheral type, setFrcParamRequest.PeripheralType())
				<< NAME_PAR(Node address, setFrcParamRequest.NodeAddress())
				<< NAME_PAR(Command, (int)setFrcParamRequest.PeripheralCommand())
			);
			removeBondResult.addTransactionResult(transResult);
			TRC_FUNCTION_LEAVE("");
			return dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSetParams_RequestResponse.FrcParams;
		} catch (const std::exception& e) {
			removeBondResult.setStatus(transResult->getErrorCode(), e.what());
			removeBondResult.addTransactionResult(transResult);
			THROW_EXC(std::logic_error, e.what());
		}
	}

	std::set<uint8_t> RemoveBondService::FRCAcknowledgedBroadcastBits(RemoveBondResult& removeBondResult, const uint8_t PNUM, const uint8_t PCMD, const uint16_t hwpId, const std::set<uint8_t> &nodes) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			// Prepare DPA request
			DpaMessage frcAckBroadcastRequest;
			DpaMessage::DpaPacket_t frcAckBroadcastPacket;
			frcAckBroadcastPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			frcAckBroadcastPacket.DpaRequestPacket_t.PNUM = PNUM_FRC;
			frcAckBroadcastPacket.DpaRequestPacket_t.PCMD = CMD_FRC_SEND_SELECTIVE;
			frcAckBroadcastPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			// FRC - Acknowledge Broadcast - Bits
			frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand = FRC_AcknowledgedBroadcastBits;
			// Select nodes
			std::vector<uint8_t> selectedNodes(30, 0);
			for (auto nodeItr = nodes.begin(); nodeItr != nodes.end(); ++nodeItr) {
				selectedNodes[*nodeItr / 8] |= (1 << (*nodeItr % 8));
			}
			std::copy(selectedNodes.begin(), selectedNodes.end(), frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes);
			// Clear UserData
			memset((void*)frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData, 0, sizeof(frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData));
			// DPA request
			frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0] = (uint8_t)(5 * sizeof(uint8_t));
			frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[1] = PNUM;
			frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[2] = PCMD;
			frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[3] = hwpId & 0xff;
			frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[4] = hwpId >> 0x08;
			// Data to buffer
			uint8_t requestLength = sizeof(TDpaIFaceHeader);
			requestLength += sizeof(frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.FrcCommand);
			requestLength += sizeof(frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.SelectedNodes);
			requestLength += frcAckBroadcastPacket.DpaRequestPacket_t.DpaMessage.PerFrcSendSelective_Request.UserData[0x00];
			frcAckBroadcastRequest.DataToBuffer(frcAckBroadcastPacket.Buffer, requestLength);
			// Execute the DPA request
			m_exclusiveAccess->executeDpaTransactionRepeat(frcAckBroadcastRequest, transResult, m_requestParams.repeat);
			TRC_DEBUG("Result from FRC_AcknowledgedBroadcastBits transaction as string:" << PAR(transResult->getErrorString()));
			DpaMessage dpaResponse = transResult->getResponse();
			TRC_INFORMATION("FRC_AcknowledgedBroadcastBits OK.");
			TRC_DEBUG(
				"DPA transaction: "
				<< NAME_PAR(Peripheral type, frcAckBroadcastRequest.PeripheralType())
				<< NAME_PAR(Node address, frcAckBroadcastRequest.NodeAddress())
				<< NAME_PAR(Command, (int)frcAckBroadcastRequest.PeripheralCommand())
			);
			// Check FRC status
			uint8_t status = dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.Status;
			if (status <= MAX_ADDRESS) {
				TRC_INFORMATION("FRC_AcknowledgedBroadcastBits OK." << NAME_PAR_HEX("Status", (int)status));
				// Return nodes that executed DPA request (bit0 is set - the DPA Request is executed.)
				std::set<uint8_t> acknowledgedNodes;
				acknowledgedNodes.clear();
				for (uint8_t nodeAddr = 1; nodeAddr <= MAX_ADDRESS; nodeAddr++) {
					if ((dpaResponse.DpaPacket().DpaResponsePacket_t.DpaMessage.PerFrcSend_Response.FrcData[nodeAddr / 8] & (1 << (nodeAddr % 8))) != 0) {
						acknowledgedNodes.insert(nodeAddr);
					}
				}
				// Add FRC result
				removeBondResult.addTransactionResult(transResult);
				TRC_FUNCTION_LEAVE("");
				return acknowledgedNodes;
			} else {
				TRC_WARNING("FRC_AcknowledgedBroadcastBits NOK." << NAME_PAR_HEX("Status", (int)status));
				THROW_EXC(std::logic_error, "Bad FRC status: " << PAR((int)status));
			}
		} catch (const std::exception& e) {
			removeBondResult.setStatus(transResult->getErrorCode(), e.what());
			removeBondResult.addTransactionResult(transResult);
			THROW_EXC(std::logic_error, e.what());
		}
	}

	void RemoveBondService::coordRemoveBond(RemoveBondResult& removeBondResult, const uint8_t nodeAddr) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			// Prepare DPA request
			DpaMessage removeBondRequest;
			DpaMessage::DpaPacket_t removeBondPacket;
			removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			removeBondPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_REMOVE_BOND;
			removeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			removeBondPacket.DpaRequestPacket_t.DpaMessage.PerCoordinatorRemoveBond_Request.BondAddr = (uint8_t)nodeAddr;
			removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + sizeof(TPerCoordinatorRemoveBond_Request));
			// Execute the DPA request
			m_exclusiveAccess->executeDpaTransactionRepeat(removeBondRequest, transResult, m_requestParams.repeat);
			TRC_DEBUG("Result from CMD_COORDINATOR_REMOVE_BOND transaction as string:" << PAR(transResult->getErrorString()));
			DpaMessage dpaResponse = transResult->getResponse();
			TRC_INFORMATION("CMD_COORDINATOR_REMOVE_BOND OK.");
			TRC_DEBUG(
				"DPA transaction: "
				<< NAME_PAR(Peripheral type, removeBondRequest.PeripheralType())
				<< NAME_PAR(Node address, removeBondRequest.NodeAddress())
				<< NAME_PAR(Command, (int)removeBondRequest.PeripheralCommand())
			);
			removeBondResult.addTransactionResult(transResult);
			TRC_FUNCTION_LEAVE("");
		} catch (const std::exception& e) {
			removeBondResult.setStatus(transResult->getErrorCode(), e.what());
			removeBondResult.addTransactionResult(transResult);
			THROW_EXC(std::logic_error, e.what());
		}
	}

	void RemoveBondService::coordRemoveBondBatch(RemoveBondResult& removeBondResult, std::set<uint8_t> &nodes) {
		TRC_FUNCTION_ENTER("");
		if (nodes.size() == 0) {
			TRC_FUNCTION_LEAVE("");
			return;
		}
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			auto nodeItr = nodes.begin();
			do {
				// Prepare DPA request
				DpaMessage removeBondRequest;
				DpaMessage::DpaPacket_t removeBondPacket;
				removeBondPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
				removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_OS;
				removeBondPacket.DpaRequestPacket_t.PCMD = CMD_OS_BATCH;
				removeBondPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
				uint8_t index = 0x00;
				uint8_t numNodes = 0x00;
				do {
					removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = 0x06;
					removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = COORDINATOR_ADDRESS;
					removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = CMD_COORDINATOR_REMOVE_BOND;
					removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = HWPID_DoNotCheck & 0xFF;
					removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = (HWPID_DoNotCheck >> 0x08);
					removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = *nodeItr++;
				} while ((++numNodes < 9) && (nodeItr != nodes.end()));
				removeBondPacket.DpaRequestPacket_t.DpaMessage.Request.PData[index++] = 0x00;
				removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader) + index);
				// Execute the DPA request
				m_exclusiveAccess->executeDpaTransactionRepeat(removeBondRequest, transResult, m_requestParams.repeat);
				TRC_DEBUG("Result from CMD_OS_BATCH transaction as string:" << PAR(transResult->getErrorString()));
				DpaMessage dpaResponse = transResult->getResponse();
				TRC_INFORMATION("CMD_OS_BATCH OK.");
				TRC_DEBUG(
					"DPA transaction: "
					<< NAME_PAR(Peripheral type, removeBondRequest.PeripheralType())
					<< NAME_PAR(Node address, removeBondRequest.NodeAddress())
					<< NAME_PAR(Command, (int)removeBondRequest.PeripheralCommand())
				);
				removeBondResult.addTransactionResult(transResult);
				std::this_thread::sleep_for(std::chrono::milliseconds(numNodes * m_coordinatorRemoveBondTimeout));
			} while (nodeItr != nodes.end());
			TRC_FUNCTION_LEAVE("");
		} catch (const std::exception& e) {
			removeBondResult.setStatus(transResult->getErrorCode(), e.what());
			removeBondResult.addTransactionResult(transResult);
			THROW_EXC(std::logic_error, e.what());
		}
	}

	void RemoveBondService::clearAllBonds(RemoveBondResult& removeBondResult) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			// Prepare DPA request
			DpaMessage clearAllBondsRequest;
			DpaMessage::DpaPacket_t clearAllBondsPacket;
			clearAllBondsPacket.DpaRequestPacket_t.NADR = COORDINATOR_ADDRESS;
			clearAllBondsPacket.DpaRequestPacket_t.PNUM = PNUM_COORDINATOR;
			clearAllBondsPacket.DpaRequestPacket_t.PCMD = CMD_COORDINATOR_CLEAR_ALL_BONDS;
			clearAllBondsPacket.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;
			clearAllBondsRequest.DataToBuffer(clearAllBondsPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute the DPA request
			m_exclusiveAccess->executeDpaTransactionRepeat(clearAllBondsRequest, transResult, m_requestParams.repeat);
			TRC_DEBUG("Result from CMD_COORDINATOR_CLEAR_ALL_BONDS transaction as string:" << PAR(transResult->getErrorString()));
			DpaMessage dpaResponse = transResult->getResponse();
			TRC_INFORMATION("CMD_COORDINATOR_CLEAR_ALL_BONDS OK.");
			TRC_DEBUG(
				"DPA transaction: "
				<< NAME_PAR(Peripheral type, clearAllBondsRequest.PeripheralType())
				<< NAME_PAR(Node address, clearAllBondsRequest.NodeAddress())
				<< NAME_PAR(Command, (int)clearAllBondsRequest.PeripheralCommand())
			);
			removeBondResult.addTransactionResult(transResult);
			TRC_FUNCTION_LEAVE("");
		} catch (const std::exception& e) {
			removeBondResult.setStatus(transResult->getErrorCode(), e.what());
			removeBondResult.addTransactionResult(transResult);
			THROW_EXC(std::logic_error, e.what());
		}
	}

	void RemoveBondService::nodeRemoveBond(RemoveBondResult& removeBondResult, const uint8_t nodeAddr, const uint16_t hwpId) {
		TRC_FUNCTION_ENTER("");
		std::unique_ptr<IDpaTransactionResult2> transResult;
		try {
			// Prepare DPA request
			DpaMessage removeBondRequest;
			DpaMessage::DpaPacket_t removeBondPacket;
			removeBondPacket.DpaRequestPacket_t.NADR = nodeAddr;
			removeBondPacket.DpaRequestPacket_t.PNUM = PNUM_NODE;
			removeBondPacket.DpaRequestPacket_t.PCMD = CMD_NODE_REMOVE_BOND;
			removeBondPacket.DpaRequestPacket_t.HWPID = hwpId;
			removeBondRequest.DataToBuffer(removeBondPacket.Buffer, sizeof(TDpaIFaceHeader));
			// Execute the DPA request
			m_exclusiveAccess->executeDpaTransactionRepeat(removeBondRequest, transResult, m_requestParams.repeat);
			TRC_DEBUG("Result from CMD_NODE_REMOVE_BOND_ADDRESS transaction as string:" << PAR(transResult->getErrorString()));
			DpaMessage dpaResponse = transResult->getResponse();
			TRC_INFORMATION("CMD_NODE_REMOVE_BOND_ADDRESS OK.");
			TRC_DEBUG(
				"DPA transaction: "
				<< NAME_PAR(Peripheral type, removeBondRequest.PeripheralType())
				<< NAME_PAR(Node address, removeBondRequest.NodeAddress())
				<< NAME_PAR(Command, (int)removeBondRequest.PeripheralCommand())
			);
			removeBondResult.addTransactionResult(transResult);
			TRC_FUNCTION_LEAVE("");
		} catch (const std::exception& e) {
			removeBondResult.setStatus(transResult->getErrorCode(), e.what());
			removeBondResult.addTransactionResult(transResult);
			THROW_EXC(std::logic_error, e.what());
		}
	}

	void RemoveBondService::removeBond(RemoveBondResult& removeBondResult) {
		TRC_FUNCTION_ENTER("");
		try {
			std::set<uint8_t> bondedNodes = getBondedNodes(removeBondResult);
			// Get DPA version
			IIqrfDpaService::CoordinatorParameters coordParams = m_dpaService->getCoordinatorParameters();
			// Broadcast address or multiple nodes
			if (m_requestParams.allNodes || m_requestParams.deviceAddrList.size() > 1) {
				// Set FRC response time 40 ms
				uint8_t initialFrcResponseTime = setFrcReponseTime(removeBondResult, IDpaTransaction2::FrcResponseTime::k40Ms);
				std::set<uint8_t> nodes;
				if (m_requestParams.allNodes) {
					nodes = bondedNodes;
					for (auto node : nodes) {
						// nodes to be removed are bonded
						removeBondResult.addNodeStatus(node, true, false);
					}
				} else {
					// get intersection
					std::set_intersection(
						bondedNodes.begin(),
						bondedNodes.end(),
						m_requestParams.deviceAddrList.begin(),
						m_requestParams.deviceAddrList.end(),
						std::inserter(nodes, nodes.begin())
					);
					for (auto node : m_requestParams.deviceAddrList) {
						// requested nodes
						removeBondResult.addNodeStatus(node, false, false);
					}
					for (auto node : nodes) {
						// bonded nodes
						removeBondResult.setNodeBonded(node, true);
					}
				}
				// FRCAcknowledgedBroadcastBits
				std::set<uint8_t> removedNodes = FRCAcknowledgedBroadcastBits(
					removeBondResult,
					PNUM_NODE,
					CMD_NODE_REMOVE_BOND,
					m_requestParams.hwpId,
					nodes
				);
				// Switch FRC response time back to initial value
				setFrcReponseTime(removeBondResult, initialFrcResponseTime);
				// Remove the sucessfully removed nodes at C side too
				coordRemoveBondBatch(removeBondResult, removedNodes);
				// Mark nodes removed
				for (auto node : removedNodes) {
					removeBondResult.setNodeRemoved(node, true);
				}
				if (nodes.size() != removedNodes.size()) {
					removeBondResult.setStatus(partailFailureError, "Some devices could not be removed from network.");
				}
				// Invoke DB enum
				invokeDbEnumeration();
			} else {
				// No, remove specified node (unicast request)
				const uint8_t nodeAddr = *m_requestParams.deviceAddrList.begin();
				// Store node bonded status
				removeBondResult.addNodeStatus(nodeAddr, false, false);
				if (bondedNodes.find(nodeAddr) != bondedNodes.end()) {
					// Store node bonded status
					removeBondResult.setNodeBonded(nodeAddr, true);
					// Remove bond at N side
					nodeRemoveBond(removeBondResult, nodeAddr, m_requestParams.hwpId);
					// Remove node at C side
					coordRemoveBond(removeBondResult, nodeAddr);
					// Store node success
					removeBondResult.setNodeRemoved(nodeAddr, true);
					// Invoke DB enum
					invokeDbEnumeration();
				} else {
					removeBondResult.setStatus(noDeviceError, "No device exists at address " + std::to_string(nodeAddr) + ".");
				}
			}
			// Get addressing info
			getAddressingInfo(removeBondResult);
			TRC_FUNCTION_LEAVE("");
		} catch (const std::exception& e) {
			CATCH_EXC_TRC_WAR(std::exception, e, e.what());
			TRC_FUNCTION_LEAVE("");
		}
	}

	void RemoveBondService::removeBondOnlyInC(RemoveBondResult& removeBondResult) {
		TRC_FUNCTION_ENTER("");
		try {
			// Get DPA version
			IIqrfDpaService::CoordinatorParameters coordParams = m_dpaService->getCoordinatorParameters();
			// Get bonded nodes
			std::set<uint8_t> bondedNodes = getBondedNodes(removeBondResult);
			// Only clear bonds if there are bonded nodes
			if (bondedNodes.size() > 0) {
				// Clear all bonds ?
				if (m_requestParams.allNodes) {
					// Yes, clear all bonds
					clearAllBonds(removeBondResult);
				} else {
					// No, remove specified nodes only
					if (m_requestParams.deviceAddrList.size() > 0) {
						// One Node in the list ?
						if (m_requestParams.deviceAddrList.size() == 1) {
							auto nodeAddr = *m_requestParams.deviceAddrList.begin();
							removeBondResult.addNodeStatus(nodeAddr, bondedNodes.count(nodeAddr) > 0, false);
							coordRemoveBond(removeBondResult, nodeAddr);
							removeBondResult.setNodeRemoved(nodeAddr, true);
						} else {
							coordRemoveBondBatch(removeBondResult, m_requestParams.deviceAddrList);
						}
					}
				}
				// Invoke DB enum
				invokeDbEnumeration();
			}
			// Get addressing info
			getAddressingInfo(removeBondResult);
			TRC_FUNCTION_LEAVE("");
		} catch (const std::exception& e) {
			CATCH_EXC_TRC_WAR(std::exception, e, e.what());
			TRC_FUNCTION_LEAVE("");
		}
	}

	void RemoveBondService::invokeDbEnumeration() {
		if (m_dbService != nullptr) {
			IIqrfDb::EnumParams parameters;
			parameters.reenumerate = true;
			parameters.standards = true;
			m_dbService->enumerate(parameters);
		}
	}

	void RemoveBondService::createResponse(const int status, const std::string &statusStr) {
		Document response;
		// Set common parameters
		Pointer("/mType").Set(response, m_msgType->m_type);
		Pointer("/data/msgId").Set(response, m_comRemoveBond->getMsgId());
		// Set status
		Pointer("/data/status").Set(response, status);
		Pointer("/data/statusStr").Set(response, statusStr);
		// Send message
		m_splitterService->sendMessage(*m_messagingId, std::move(response));
	}

	void RemoveBondService::createResponse(RemoveBondResult& removeBondResult) {
		Document response;
		// Set common parameters
		Pointer("/mType").Set(response, m_msgType->m_type);
		Pointer("/data/msgId").Set(response, m_comRemoveBond->getMsgId());
		// Set status
		int status = removeBondResult.getStatus();
		// Rsp object
		rapidjson::Pointer("/data/rsp/nodesNr").Set(response, removeBondResult.getNodesNr());
		// iqmeshNetwork_RemoveBond request
		if (!m_requestParams.coordinatorOnly) {
			Document::AllocatorType& allocator = response.GetAllocator();
			rapidjson::Value nodeArray(kArrayType);
			auto nodes = removeBondResult.getNodesStatus();
			for (auto &[addr, v] : nodes) {
				rapidjson::Value nodeObject(kObjectType);
				Pointer("/address").Set(nodeObject, addr, allocator);
				Pointer("/bonded").Set(nodeObject, v.bonded, allocator);
				Pointer("/removed").Set(nodeObject, v.removed, allocator);
				nodeArray.PushBack(nodeObject, allocator);
			}
			Pointer("/data/rsp/result").Set(response, nodeArray);
		}

		// Set raw fields, if verbose mode is active
		if (m_comRemoveBond->getVerbose()) {
			rapidjson::Value rawArray(kArrayType);
			Document::AllocatorType& allocator = response.GetAllocator();
			while (removeBondResult.isNextTransactionResult()) {
				std::unique_ptr<IDpaTransactionResult2> transResult = removeBondResult.consumeNextTransactionResult();
				rapidjson::Value rawObject(kObjectType);
				rawObject.AddMember(
					"request",
					HexStringConversion::encodeBinary(transResult->getRequest().DpaPacket().Buffer, transResult->getRequest().GetLength()),
					allocator
				);
				rawObject.AddMember(
					"requestTs",
					TimeConversion::encodeTimestamp(transResult->getRequestTs()),
					allocator
				);
				rawObject.AddMember(
					"confirmation",
					HexStringConversion::encodeBinary(transResult->getConfirmation().DpaPacket().Buffer, transResult->getConfirmation().GetLength()),
					allocator
				);
				rawObject.AddMember(
					"confirmationTs",
					TimeConversion::encodeTimestamp(transResult->getConfirmationTs()),
					allocator
				);
				rawObject.AddMember(
					"response",
					HexStringConversion::encodeBinary(transResult->getResponse().DpaPacket().Buffer, transResult->getResponse().GetLength()),
					allocator
				);
				rawObject.AddMember(
					"responseTs",
					TimeConversion::encodeTimestamp(transResult->getResponseTs()),
					allocator
				);
				// add object into array
				rawArray.PushBack(rawObject, allocator);
			}
			// Add array into response document
			Pointer("/data/raw").Set(response, rawArray);
		}
		// Set status
		Pointer("/data/status").Set(response, status);
		Pointer("/data/statusStr").Set(response, removeBondResult.getStatusStr());
		// Send message
		m_splitterService->sendMessage(*m_messagingId, std::move(response));
	}

	///// Component management

	void RemoveBondService::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"************************************" << std::endl <<
			"RemoveBondService instance activate" << std::endl <<
			"************************************"
		);
		(void)props;
		// for the sake of register function parameters
		std::vector<std::string> supportedMsgTypes = {
			m_mTypeName_iqmeshNetworkRemoveBond,
		};
		m_splitterService->registerFilteredMsgHandler(
			supportedMsgTypes,
			[&](const std::string & messagingId, const IMessagingSplitterService::MsgType & msgType, rapidjson::Document doc) {
				handleMsg(messagingId, msgType, std::move(doc));
			}
		);
		TRC_FUNCTION_LEAVE("")
	}

	void RemoveBondService::modify(const shape::Properties *props) {
		(void)props;
	}

	void RemoveBondService::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"************************************" << std::endl <<
			"RemoveBondService instance deactivate" << std::endl <<
			"************************************"
		);
		std::vector<std::string> supportedMsgTypes = {
			m_mTypeName_iqmeshNetworkRemoveBond
		};
		m_splitterService->unregisterFilteredMsgHandler(supportedMsgTypes);
		TRC_FUNCTION_LEAVE("");
	}

	///// Interface management

	void RemoveBondService::attachInterface(IIqrfDb *iface) {
			m_dbService = iface;
		}

	void RemoveBondService::detachInterface(IIqrfDb *iface) {
		if (m_dbService == iface) {
			m_dbService = nullptr;
		}
	}

	void RemoveBondService::attachInterface(IIqrfDpaService* iface) {
		m_dpaService = iface;
	}

	void RemoveBondService::detachInterface(IIqrfDpaService* iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void RemoveBondService::attachInterface(IMessagingSplitterService* iface) {
		m_splitterService = iface;
	}

	void RemoveBondService::detachInterface(IMessagingSplitterService* iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void RemoveBondService::attachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void RemoveBondService::detachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
