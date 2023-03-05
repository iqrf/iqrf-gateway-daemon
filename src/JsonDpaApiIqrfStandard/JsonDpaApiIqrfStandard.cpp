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

#define IMessagingSplitterService_EXPORTS


#include "JsonDpaApiIqrfStandard.h"
#include "iqrf__JsonDpaApiIqrfStandard.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonDpaApiIqrfStandard)

using namespace rapidjson;

namespace iqrf {

	JsonDpaApiIqrfStandard::JsonDpaApiIqrfStandard() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	JsonDpaApiIqrfStandard::~JsonDpaApiIqrfStandard() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	///// Component lifecycle methods

	void JsonDpaApiIqrfStandard::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"JsonDpaApiIqrfStandard instance activate" << std::endl <<
			"******************************"
		);
		modify(props);
		m_splitterService->registerFilteredMsgHandler(m_filters, [&](const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc) {
			handleMsg(messagingId, msgType, std::move(doc));
		});
		m_dpaService->registerAsyncMessageHandler(m_instance, [&](const DpaMessage &dpaMessage) {
			handleAsyncMsg(dpaMessage);
		});
		TRC_FUNCTION_LEAVE("");
	}

	void JsonDpaApiIqrfStandard::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		const Document &doc = props->getAsJson();
		m_instance = Pointer("/instance").Get(doc)->GetString();
		TRC_FUNCTION_LEAVE("");
	}

	void JsonDpaApiIqrfStandard::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl <<
			"******************************" << std::endl <<
			"JsonDpaApiIqrfStandard instance deactivate" << std::endl <<
			"******************************"
		);

		{
			std::lock_guard<std::mutex> lock(m_transactionMutex);
			if (m_transaction) {
				m_transaction->abort();
			}
		}

		m_splitterService->unregisterFilteredMsgHandler(m_filters);
		m_dpaService->unregisterAsyncMessageHandler(m_instance);
		TRC_FUNCTION_LEAVE("");
	}

	///// Message handling

	void JsonDpaApiIqrfStandard::handleMsg(const std::string &messagingId, const IMessagingSplitterService::MsgType &msgType, Document doc) {
		TRC_FUNCTION_ENTER(
			PAR(messagingId) <<
			NAME_PAR(mType, msgType.m_type) <<
			NAME_PAR(major, msgType.m_major) <<
			NAME_PAR(minor, msgType.m_minor) <<
			NAME_PAR(micro, msgType.m_micro)
		);

		Document allResponseDoc;
		std::unique_ptr<ComIqrfStandard> com(shape_new ComIqrfStandard(doc));

		if (m_dbService && m_dbService->addMetadataToMessage()) {
			Document metaDataDoc;
			try {
				metaDataDoc = m_dbService->getDeviceMetadataDoc(com->getNadr());
			} catch (const std::exception &e) {
				TRC_WARNING(e.what());
			}
			com->setMidMetaData(metaDataDoc);
		}

		std::string methodRequestName = msgType.m_possibleDriverFunction;
		std::string methodResponseName = msgType.m_possibleDriverFunction;
		methodRequestName += "_Request_req";
		methodResponseName += "_Response_rsp";

		// call request driver func, it returns rawHdpRequest format in text form
		std::string rawHdpRequest;
		std::string errStrReq;
		bool driverRequestError = false;
    int errorCode = 0;
		try {
			m_jsRenderService->callContext(com->getNadr(), com->getHwpid(), methodRequestName, com->getParamAsString(), rawHdpRequest);
    } catch (const PeripheralException &e) {
      CATCH_EXC_TRC_WAR(PeripheralException, e, e.what());
      errorCode = ERROR_PNUM;
      errStrReq = "Invalid PNUM";
      driverRequestError = true;
    } catch (const PeripheralCommandException &e) {
      CATCH_EXC_TRC_WAR(PeripheralCommandException, e, e.what());
      errorCode = ERROR_PCMD;
      errStrReq = "Invalid PCMD";
      driverRequestError = true;
		} catch (std::exception &e) {
			//request driver func error
			errStrReq = e.what();
			driverRequestError = true;
		}

		if (driverRequestError) {
      com->setRequestDriverConvertFailure(true);
      try {
        if (msgType.m_type == "iqrfEmbedExplore_PeripheralInformation" || msgType.m_type == "iqrfEmbedExplore_MorePeripheralsInformation") {
          try {
            auto per = static_cast<uint8_t>(Pointer("/data/req/param/per").Get(doc)->GetUint());
            if (msgType.m_type == "iqrfEmbedExplore_PeripheralInformation") {
              com->setPnum(per);
              com->setPcmd(EnumUtils::toScalar(message_types::Commands::Exploration_PerInfo));
            } else if (msgType.m_type == "iqrfEmbedExplore_MorePeripheralsInformation") {
              com->setPnum(EnumUtils::toScalar(message_types::Peripherals::Exploration));
              com->setPcmd(per);
            }
          } catch (const std::exception &e) {
            CATCH_EXC_TRC_WAR(std::exception, e, e.what());
            com->setUnresolvablePerCmd(true);
          }
        } else if (msgType.m_type == "iqrfEmbedLedr_Set" || msgType.m_type == "iqrfEmbedLedg_Set") {
          auto pnum = msgType.m_type == "iqrfEmbedLedr_Set" ? message_types::Peripherals::Ledr : message_types::Peripherals::Ledg;
          auto cmd = static_cast<uint8_t>(Pointer("/data/req/param/onOff").Get(doc)->GetBool());
          com->setPnum(EnumUtils::toScalar(pnum));
          com->setPcmd(cmd);
        } else {
          auto tuple = message_types::MessageTypeConverter::messageTypeToPerCmd(msgType.m_type);
          com->setPnum(EnumUtils::toScalar(std::get<0>(tuple)));
          com->setPcmd(EnumUtils::toScalar(std::get<1>(tuple)));
        }
      } catch (const std::domain_error &e) {
        com->setUnresolvablePerCmd(true);
      }
			//provide error response
			FakeTransactionResult fr(com->getDpaRequest(), true);
      fr.setErrorString(errStrReq);
      if (errorCode != 0) {
        fr.overrideErrorCode(static_cast<IDpaTransactionResult2::ErrorCode>(errorCode));
      }
			com->setStatus(fr.getErrorString(), fr.getErrorCode());
			com->createResponse(allResponseDoc, fr);
		} else {
			TRC_DEBUG(PAR(rawHdpRequest));
			// convert from rawHdpRequest to dpaRequest and pass nadr and hwpid to be in dapaRequest (driver doesn't set them)
			int hwpidReq = com->getHwpid();
			std::vector<uint8_t> dpaRequest = hdpToDpa(com->getNadr(), hwpidReq < 0 ? 0xffff : hwpidReq, rawHdpRequest);

			// setDpaRequest as DpaMessage in com object
			com->setDpaMessage(dpaRequest);

			// send to coordinator and wait for transaction result
			{
				std::lock_guard<std::mutex> lck(m_transactionMutex);
				m_transaction = m_dpaService->executeDpaTransaction(com->getDpaRequest(), com->getTimeout());
			}
			auto res = m_transaction->get();


			//process response
			int nadrRes = com->getNadr();
			int hwpidRes = 0;
			int rcode = -1;

			if (res->isResponded()) {
				//we have some response
				const uint8_t *buf = res->getResponse().DpaPacket().Buffer;
				int sz = res->getResponse().GetLength();
				std::vector<uint8_t> dpaResponse(buf, buf + sz);

				// get rawHdpResponse in text form
				std::string rawHdpResponse;
				// original rawHdpRequest request passed for additional sensor breakdown parsing
				// TODO it is not necessary for all other handling, may be optimized in future
				rawHdpResponse = dpaToHdp(nadrRes, hwpidRes, rcode, dpaResponse, rawHdpRequest);
				TRC_DEBUG(PAR(rawHdpResponse))

				if (0 == rcode) {
					// call response driver func, it returns rsp{} in text form
					std::string rspObjStr;
					std::string errStrRes;
					bool driverResponseError = false;
					try {
						m_jsRenderService->callContext(nadrRes, hwpidRes, methodResponseName, rawHdpResponse, rspObjStr);
					} catch (std::exception &e) {
						//response driver func error
						errStrRes = e.what();
						driverResponseError = true;
					}

					if (driverResponseError) {
						//provide error response
						Document rDataError;
						rDataError.SetString(errStrRes.c_str(), rDataError.GetAllocator());
						com->setPayload("/data/rsp/errorStr", rDataError, true);
						res->overrideErrorCode(IDpaTransactionResult2::ErrorCode::TRN_ERROR_BAD_RESPONSE);
						com->setStatus(res->getErrorString(), res->getErrorCode());
						com->createResponse(allResponseDoc, *res);
					} else {
						// store in db
						if (m_dbService && msgType.m_type == "iqrfSensor_ReadSensorsWithTypes") {
							m_dbService->updateSensorValues(nadrRes, rspObjStr);
						}
						// get json from its text representation
						Document rspObj;
						rspObj.Parse(rspObjStr);
						TRC_DEBUG("result object: " << std::endl << jsonToStr(&rspObj));
						com->setPayload("/data/rsp/result", rspObj, false);
						com->setStatus(res->getErrorString(), res->getErrorCode());
						com->createResponse(allResponseDoc, *res);
					}
				} else {
					Document rDataError;
					rDataError.SetString("rcode error", rDataError.GetAllocator());
					com->setPayload("/data/rsp/errorStr", rDataError, true);
					com->setStatus(res->getErrorString(), res->getErrorCode());
					com->createResponse(allResponseDoc, *res);
				}
			} else {
				if (res->getErrorCode() != 0) {
					Document rDataError;
					rDataError.SetString("rcode error", rDataError.GetAllocator());
					com->setPayload("/data/rsp/errorStr", rDataError, true);
					com->setStatus(res->getErrorString(), res->getErrorCode());
					com->createResponse(allResponseDoc, *res);
				} else {
					//no response but not considered as an error
					Document rspObj;
					Pointer("/response").Set(rspObj, "unrequired");
					com->setPayload("/data/rsp/result", rspObj, false);
					com->setStatus(res->getErrorString(), res->getErrorCode());
					com->createResponse(allResponseDoc, *res);
				}
			}
		}
		TRC_DEBUG("response object: " << std::endl << jsonToStr(&allResponseDoc));

		m_splitterService->sendMessage(messagingId, std::move(allResponseDoc));

		TRC_FUNCTION_LEAVE("");
	}

	void JsonDpaApiIqrfStandard::handleAsyncMsg(const DpaMessage &dpaMessage) {
		TRC_FUNCTION_ENTER("");

		try {
			using namespace rapidjson;

			// parse raw data to basic response structure
			iqrf::raw::AnyAsyncResponse anyAsyncResponse(dpaMessage);
			const uint8_t *buf = anyAsyncResponse.getResponse().DpaPacket().Buffer;
			int sz = anyAsyncResponse.getResponse().GetLength();
			std::vector<uint8_t> dpaResponse(buf, buf + sz);

			// deduce driver function from PNUM, PCMD
			std::string methodResponseName;
			std::string mTypeName;
			std::string payloadKey;

			if (anyAsyncResponse.getPnum() == 0x5E) { //std sensor
				if (anyAsyncResponse.getPcmd() == 0x7B) { //ReadSensorsWithTypes
					methodResponseName = "iqrf.sensor.ReadSensorsWithTypesFrcValue_AsyncResponse";
					mTypeName = "iqrfSensor_ReadSensorsWithTypes";
					payloadKey = "/data/rsp/result/sensors";
				} else { // if (anyAsyncResponse.getPcmd() == ??) {} // add here possible other supported pcmd + matching driver function
					THROW_EXC(std::invalid_argument, "Unsupported PCMD: " << std::to_string(anyAsyncResponse.getPcmd()));
				}
			} else {
				THROW_EXC(std::invalid_argument, "Unsupported PNUM: " << std::to_string(anyAsyncResponse.getPnum()));
			}

			static int asyncCnt = 1;
			std::ostringstream msgIdOs;
			msgIdOs << "async-" << asyncCnt++;

			Document fakeRequest;

			Pointer("/mType").Set(fakeRequest, mTypeName);
			Pointer("/data/msgId").Set(fakeRequest, msgIdOs.str());
			Pointer("/data/req/nAdr").Set(fakeRequest, anyAsyncResponse.getNadr());
			Pointer("/data/req/hwpId").Set(fakeRequest, anyAsyncResponse.getHwpid());
			Pointer("/data/req/param").Set(fakeRequest, Value(kObjectType));
			// hardcoded verbose
			Pointer("/data/returnVerbose").Set(fakeRequest, true);

			std::unique_ptr<ComIqrfStandard> com(shape_new ComIqrfStandard(fakeRequest));
			std::unique_ptr<FakeTransactionResult> res(shape_new FakeTransactionResult(dpaMessage));

			Document allResponseDoc;

			if (m_dbService && m_dbService->addMetadataToMessage()) {
				Document metaDataDoc;
				try {
					metaDataDoc = m_dbService->getDeviceMetadataDoc(com->getNadr());
				} catch (const std::exception &e) {
					TRC_WARNING(e.what());
				}
				com->setMidMetaData(metaDataDoc);
			}

			//to be filled
			int nadrRes = 0;
			int hwpidRes = 0;
			int rcode = -1;

			std::string rawHdpRequest; //empty just to satisfy next method signature

			// get rawHdpResponse in text form
			std::string rawHdpResponse;
			// original rawHdpRequest request passed for additional sensor breakdown parsing
			rawHdpResponse = dpaToHdp(nadrRes, hwpidRes, rcode, dpaResponse, rawHdpRequest);
			TRC_DEBUG(PAR(rawHdpResponse))

			if (anyAsyncResponse.isAsyncRcode() && 0 == (rcode & 0x7f)) {
				// call response driver func, it returns rsp{} in text form
				std::string rspObjStr;
				std::string errStrRes;
				bool driverResponseError = false;
				try {
					m_jsRenderService->callContext(nadrRes, hwpidRes, methodResponseName, rawHdpResponse, rspObjStr);
				} catch (std::exception &e) {
					//response driver func error
					errStrRes = e.what();
					driverResponseError = true;
				}

				if (driverResponseError) {
					//provide error response
					Document rDataError;
					rDataError.SetString(errStrRes.c_str(), rDataError.GetAllocator());
					com->setPayload("/data/rsp/errorStr", rDataError, true);

					//fake transaction result
					res->overrideErrorCode(IDpaTransactionResult2::ErrorCode::TRN_ERROR_BAD_RESPONSE);
					res->setErrorString("BAD_RESPONSE");

					com->setStatus(res->getErrorString(), res->getErrorCode());
					com->createResponse(allResponseDoc, *res);
				} else {
					// get json from its text representation
					Document rspObj;
					rspObj.Parse(rspObjStr);
					TRC_DEBUG("result object: " << std::endl << jsonToStr(&rspObj));

					//fake transaction result
					res->overrideErrorCode(IDpaTransactionResult2::ErrorCode::TRN_OK);
					res->setErrorString("ok");

					com->setPayload(payloadKey, rspObj, false);
					com->setStatus(res->getErrorString(), res->getErrorCode());
					com->createResponse(allResponseDoc, *res);
				}
			} else {
				Document rDataError;
				rDataError.SetString("rcode error", rDataError.GetAllocator());
				com->setPayload("/data/rsp/errorStr", rDataError, true);

				//fake transaction result
				res->overrideErrorCode(IDpaTransactionResult2::ErrorCode::TRN_ERROR_BAD_RESPONSE);
				res->setErrorString("BAD_RESPONSE rcode");

				com->setStatus(res->getErrorString(), res->getErrorCode());
				com->createResponse(allResponseDoc, *res);
			}

			TRC_DEBUG("response object: " << std::endl << jsonToStr(&allResponseDoc));

			//empty messagingId => send to all messaging returning iface->acceptAsyncMsg() == true
			m_splitterService->sendMessage("", std::move(allResponseDoc));

		} catch (std::invalid_argument &e) {
			// unsupported PNUM, PCMD - just skip it, we don't care
			TRC_DEBUG(e.what());
		} catch (std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "Wrong format of async response");
		}

		TRC_FUNCTION_LEAVE("")
	}

	///// Auxiliary methods

	std::vector<uint8_t> JsonDpaApiIqrfStandard::hdpToDpa(uint8_t nadr, uint16_t hwpid, const std::string &hdpRequest) {
		// TODO return DPA message
		std::vector<uint8_t> retvect;

		Document doc;
		doc.Parse(hdpRequest);

		uint8_t pnum = 0, pcmd = 0;

		if (Value *val = Pointer("/pnum").Get(doc)) {
			HexStringConversion::parseHexaNum(pnum, val->GetString());
		}
		if (Value *val = Pointer("/pcmd").Get(doc)) {
			HexStringConversion::parseHexaNum(pcmd, val->GetString());
		}

		retvect.push_back(nadr & 0xff);
		retvect.push_back((nadr >> 8) & 0xff);
		retvect.push_back(pnum);
		retvect.push_back(pcmd);
		retvect.push_back(hwpid & 0xff);
		retvect.push_back((hwpid >> 8) & 0xff);

		if (Value *val = Pointer("/rdata").Get(doc)) {
			uint8_t buf[DPA_MAX_DATA_LENGTH];
			int len = HexStringConversion::parseBinary(buf, val->GetString(), DPA_MAX_DATA_LENGTH);
			for (int i = 0; i < len; i++) {
			retvect.push_back(buf[i]);
			}
		}

		return retvect;
	}

	std::string JsonDpaApiIqrfStandard::dpaToHdp(int &nadr, int &hwpid, int &rcode, const std::vector<uint8_t> &dpaResponse, const std::string &hdpRequest) {
		// TODO pass DpaMessage
		std::string rawHdpResponse;
		Document doc;

		if (dpaResponse.size() >= 8) {
			uint8_t pnum = 0, pcmd = 0, rcode8 = 0, dpaval = 0;
			std::string pnumStr, pcmdStr, rcodeStr, dpavalStr;

			nadr = dpaResponse[0];
			nadr += dpaResponse[1] << 8;
			pnum = dpaResponse[2];
			pcmd = dpaResponse[3];
			hwpid = dpaResponse[4];
			hwpid += dpaResponse[5] << 8;
			rcode8 = dpaResponse[6] & 0x7F; //reset async flag
			rcode = rcode8;
			dpaval = dpaResponse[7];

			pnumStr = HexStringConversion::encodeHexaNum(pnum);
			pcmdStr = HexStringConversion::encodeHexaNum(pcmd);
			rcodeStr = HexStringConversion::encodeHexaNum(rcode8);
			dpavalStr = HexStringConversion::encodeHexaNum(dpaval);

			//nadr, hwpid is not interesting for drivers
			Pointer("/pnum").Set(doc, pnumStr);
			Pointer("/pcmd").Set(doc, pcmdStr);
			Pointer("/rcode").Set(doc, rcodeStr);
			Pointer("/dpaval").Set(doc, rcodeStr);

			if (dpaResponse.size() > 8) {
				Pointer("/rdata").Set(doc, HexStringConversion::encodeBinary(dpaResponse.data() + 8, static_cast<int>(dpaResponse.size()) - 8));
			}

			Document rawHdpRequestDoc;
			rawHdpRequestDoc.Parse(hdpRequest);
			Pointer("/originalRequest").Set(doc, rawHdpRequestDoc);

			rawHdpResponse = jsonToStr(&doc);
		}

		return rawHdpResponse;
	}

	std::string JsonDpaApiIqrfStandard::jsonToStr(const Value* val) {
		Document doc;
		doc.CopyFrom(*val, doc.GetAllocator());
		StringBuffer buffer;
		PrettyWriter<StringBuffer> writer(buffer);
		doc.Accept(writer);
		return buffer.GetString();
	}

	///// Interface management

	void JsonDpaApiIqrfStandard::attachInterface(IIqrfDb *iface) {
		m_dbService = iface;
	}

	void JsonDpaApiIqrfStandard::detachInterface(IIqrfDb *iface) {
		if (m_dbService == iface) {
			m_dbService = nullptr;
		}
	}

	void JsonDpaApiIqrfStandard::attachInterface(IIqrfDpaService *iface){
		m_dpaService = iface;
	}

	void JsonDpaApiIqrfStandard::detachInterface(IIqrfDpaService *iface) {
		if (m_dpaService == iface) {
			m_dpaService = nullptr;
		}
	}

	void JsonDpaApiIqrfStandard::attachInterface(IJsRenderService *iface) {
		m_jsRenderService = iface;
	}

	void JsonDpaApiIqrfStandard::detachInterface(IJsRenderService *iface) {
		if (m_jsRenderService == iface) {
			m_jsRenderService = nullptr;
		}
	}

	void JsonDpaApiIqrfStandard::attachInterface(IMessagingSplitterService *iface) {
		m_splitterService = iface;
	}

	void JsonDpaApiIqrfStandard::detachInterface(IMessagingSplitterService *iface) {
		if (m_splitterService == iface) {
			m_splitterService = nullptr;
		}
	}

	void JsonDpaApiIqrfStandard::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void JsonDpaApiIqrfStandard::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
