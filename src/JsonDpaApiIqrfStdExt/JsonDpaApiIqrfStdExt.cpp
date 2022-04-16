/**
 * Copyright 2015-2024 IQRF Tech s.r.o.
 * Copyright 2019-2024 MICRORISC s.r.o.
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

#include "JsonDpaApiIqrfStdExt.h"
#include "iqrf__JsonDpaApiIqrfStdExt.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApiIqrfStdExt)

using namespace rapidjson;
using json = nlohmann::json;

namespace iqrf {

	JsonDpaApiIqrfStdExt::JsonDpaApiIqrfStdExt() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	JsonDpaApiIqrfStdExt::~JsonDpaApiIqrfStdExt() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Component lifecycle methods

	void JsonDpaApiIqrfStdExt::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"JsonDpaApiIqrfStdExt instance activate" << std::endl <<
			"******************************"
		);
		modify(props);
		m_splitterService->registerFilteredMsgHandler(m_filters,
			[&](const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc)
		{
			handleMsg(messagingId, msgType, std::move(doc));
		});

		TRC_FUNCTION_LEAVE("")
	}

	void JsonDpaApiIqrfStdExt::modify(const shape::Properties *props) {
		(void)props;
	}

	void JsonDpaApiIqrfStdExt::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"JsonDpaApiIqrfStdExt instance deactivate" << std::endl <<
			"******************************"
		);

		{
			std::lock_guard<std::mutex> lck(m_transactionMutex);
			if (m_transaction) {
				m_transaction->abort();
			}
		}

		m_splitterService->unregisterFilteredMsgHandler(m_filters);

		TRC_FUNCTION_LEAVE("")
	}

	///// Message handling

	void JsonDpaApiIqrfStdExt::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc) {
		TRC_FUNCTION_ENTER(PAR(messagingId) << NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));


		Document allResponseDoc;

		// solves api msg processing
		ApiMsgIqrfStandardFrc apiMsgIqrfStandardFrc(doc);

		try {
			// solves JsDriver processing
			JsDriverStandardFrcSolver jsDriverStandardFrcSolver(m_jsRenderService, msgType.m_possibleDriverFunction,
				apiMsgIqrfStandardFrc.getRequestParamDoc(), apiMsgIqrfStandardFrc.getHwpid());

			// process *_Request
			jsDriverStandardFrcSolver.processRequestDrv();
			apiMsgIqrfStandardFrc.setDpaRequest(jsDriverStandardFrcSolver.getFrcRequest());

			auto exclusiveAccess = m_dpaService->getExclusiveAccess();
			int timeOut = apiMsgIqrfStandardFrc.getTimeout();
			// FRC transaction
			std::unique_ptr<IDpaTransactionResult2> transResultFrc = exclusiveAccess->executeDpaTransaction(jsDriverStandardFrcSolver.getFrcRequest(), timeOut)->get();
			jsDriverStandardFrcSolver.setFrcDpaTransactionResult(std::move(transResultFrc));

			if (apiMsgIqrfStandardFrc.getExtraResult()) {
				// FRC extra result transaction
				std::unique_ptr<IDpaTransactionResult2> transResultFrcExtra = exclusiveAccess->executeDpaTransaction(jsDriverStandardFrcSolver.getFrcExtraRequest())->get();
				jsDriverStandardFrcSolver.setFrcExtraDpaTransactionResult(std::move(transResultFrcExtra));
			}

			if (m_dbService && msgType.m_type == "iqrfSensor_Frc") {
				sensor::jsdriver::SensorFrc sensorFrc(apiMsgIqrfStandardFrc.getRequestParamDoc());
				const uint8_t type = sensorFrc.getType();
				if (type == 129 || type == 160) {
					auto map = m_dbService->getSensorDeviceHwpids(type);
					jsDriverStandardFrcSolver.processResponseSensorDrv(map, sensorFrc.getSelectedNodes(), sensorFrc.getExtraResult());
				} else {
					jsDriverStandardFrcSolver.processResponseDrv();
				}
				m_dbService->updateSensorValues(sensorFrc.getType(), sensorFrc.getIndex(), sensorFrc.getSelectedNodes(), jsDriverStandardFrcSolver.getResponseResultStr());
			} else {
				jsDriverStandardFrcSolver.processResponseDrv();
			}

			json jsonDoc = json::parse(jsDriverStandardFrcSolver.getResponseResultStr());
			std::string arrayKey = apiMsgIqrfStandardFrc.getArrayKeyByMessageType(msgType.m_type);
			// selected nodes
			if (apiMsgIqrfStandardFrc.hasSelectedNodes()) {
				jsDriverStandardFrcSolver.filterSelectedNodes(
					jsonDoc,
					arrayKey,
					apiMsgIqrfStandardFrc.getSelectedNodes().size()
				);
			}
			// ext format
			if (apiMsgIqrfStandardFrc.getExtFormat()) {
				
				jsDriverStandardFrcSolver.convertToExtendedFormat(
					jsonDoc,
					arrayKey,
					apiMsgIqrfStandardFrc.getItemKeyByMessageType(msgType.m_type),
					apiMsgIqrfStandardFrc.getSelectedNodes()
				);

				if (m_dbService && m_dbService->addMetadataToMessage()) {
					try {
						for (auto itr = jsonDoc[arrayKey].begin(); itr != jsonDoc[arrayKey].end(); ++itr) {
							uint8_t nadr = (*itr)["nAdr"].get<uint8_t>();
								std::string metadataStr = jutils::jsonToStr(m_dbService->getDeviceMetadataDoc());
								(*itr)["metaData"] = json::parse(metadataStr);
						}
					} catch (const std::exception &e) {
						CATCH_EXC_TRC_WAR(std::exception, e, "Cannot annotate by metadata");
					}
				}
			}

			std::string jsonDocStr = jsonDoc.dump();
			Document doc;
			doc.Parse(jsonDocStr);
			apiMsgIqrfStandardFrc.setPayload("/data/rsp/result", doc);

			apiMsgIqrfStandardFrc.setDpaTransactionResult(jsDriverStandardFrcSolver.moveFrcDpaTransactionResult());
			apiMsgIqrfStandardFrc.setDpaTransactionExtraResult(jsDriverStandardFrcSolver.moveFrcExtraDpaTransactionResult());
			IDpaTransactionResult2::ErrorCode status = IDpaTransactionResult2::ErrorCode::TRN_OK;
			apiMsgIqrfStandardFrc.setStatus(IDpaTransactionResult2::errorCode(status), status);
			apiMsgIqrfStandardFrc.createResponse(allResponseDoc);
		}
		catch (std::exception & e) {
			//provide error response
			Document rDataError;
			rDataError.SetString(e.what(), rDataError.GetAllocator());
			apiMsgIqrfStandardFrc.setPayload("/data/rsp/errorStr", std::move(rDataError));
			//apiMsgIqrfStandardFrc.setStatus(IDpaTransactionResult2::errorCode(e.getStatus()), e.getStatus());
			apiMsgIqrfStandardFrc.setStatus(
				IDpaTransactionResult2::errorCode(IDpaTransactionResult2::ErrorCode::TRN_ERROR_FAIL), IDpaTransactionResult2::ErrorCode::TRN_ERROR_FAIL);
			apiMsgIqrfStandardFrc.createResponse(allResponseDoc);
		}

		m_splitterService->sendMessage(messagingId, std::move(allResponseDoc));

		TRC_FUNCTION_LEAVE("");
	}

	///// Interface management

	void JsonDpaApiIqrfStdExt::attachInterface(IIqrfDb *iface) {
		m_dbService = iface;
	}

	void JsonDpaApiIqrfStdExt::detachInterface(IIqrfDb *iface) {
		if (m_dbService == iface) {
			m_dbService = nullptr;
		}
	}

	void JsonDpaApiIqrfStdExt::attachInterface(IJsRenderService *iface) {
		m_jsRenderService = iface;
	}

	void JsonDpaApiIqrfStdExt::detachInterface(IJsRenderService *iface) {
		if (m_jsRenderService == iface) {
			m_jsRenderService = nullptr;
		}
	}

	void JsonDpaApiIqrfStdExt::attachInterface(IIqrfDpaService* iface){
		m_dpaService = iface;
	}

	void JsonDpaApiIqrfStdExt::detachInterface(IIqrfDpaService* iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void JsonDpaApiIqrfStdExt::attachInterface(IMessagingSplitterService* iface) {
		m_splitterService = iface;
	}

	void JsonDpaApiIqrfStdExt::detachInterface(IMessagingSplitterService* iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void JsonDpaApiIqrfStdExt::attachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void JsonDpaApiIqrfStdExt::detachInterface(shape::ITraceService* iface) {
		shape::Tracer::get().removeTracerService(iface);
	}

}
