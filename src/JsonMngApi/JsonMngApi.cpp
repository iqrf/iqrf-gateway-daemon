/**
 * IQRF Gateway Daemon
 * Copyright (C) 2015-2025 IQRF Tech s.r.o., 2019-2025 MICRORISC s.r.o.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ApiMsg.h"
#include "JsonMngApi.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "Trace.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>

#include "Messages/MngBaseMsg.h"
#include "Messages/MngExitMsg.h"
#include "Messages/MngModeMsg.h"
#include "Messages/MngReloadCoordinatorMsg.h"
#include "Messages/MngUpdateCacheMsg.h"
#include "Messages/MngVersionMsg.h"

#include "Messages/SchedulerAddTaskMsg.h"
#include "Messages/SchedulerChangeTaskStateMsg.h"
#include "Messages/SchedulerEditTaskMsg.h"
#include "Messages/SchedulerGetTaskMsg.h"
#include "Messages/SchedulerListMsg.h"
#include "Messages/SchedulerRemoveAllMsg.h"
#include "Messages/SchedulerRemoveTaskMsg.h"

#include "iqrf__JsonMngApi.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsonMngApi);

using namespace rapidjson;

namespace iqrf {

	class JsonMngApi::Imp {
	private:
		/// Launch service
		shape::ILaunchService *m_iLaunchService = nullptr;
		/// Tracing service
		shape::ITraceService *m_traceService = nullptr;
		/// IqrfDb service
		IIqrfDb *m_dbService = nullptr;
		/// DPA service
		IIqrfDpaService *m_dpaService = nullptr;
		/// Scheduler service
		ISchedulerService *m_iSchedulerService = nullptr;
		/// JsonSplitter service
		IMessagingSplitterService *m_iMessagingSplitterService = nullptr;
		/// JsCache service
		IJsCacheService *m_cacheService = nullptr;
		/// UDP service
		IUdpConnectorService *m_iUdpConnectorService = nullptr;
		/// API message filters
		std::vector<std::string> m_filters = {
			"mngScheduler",
			"mngDaemon"
		};
	public:
		Imp() {}

		~Imp() {}

		void handleMsg(const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc) {
			TRC_FUNCTION_ENTER(PAR(messaging.to_string()) << NAME_PAR(mType, msgType.m_type) << NAME_PAR(major, msgType.m_major) << NAME_PAR(minor, msgType.m_minor) << NAME_PAR(micro, msgType.m_micro));
			Document respDoc;
			std::unique_ptr<MngBaseMsg> msg;
			if (msgType.m_type == "mngDaemon_Exit") {
				msg = std::make_unique<MngExitMsg>(MngExitMsg(doc, m_iSchedulerService));
			} else if (msgType.m_type == "mngDaemon_Mode") {
				msg = std::make_unique<MngModeMsg>(MngModeMsg(doc, m_iUdpConnectorService));
			} else if (msgType.m_type == "mngDaemon_ReloadCoordinator") {
				msg = std::make_unique<MngReloadCoordinatorMsg>(MngReloadCoordinatorMsg(doc, m_dpaService, m_dbService));
			} else if (msgType.m_type == "mngDaemon_UpdateCache") {
				msg = std::make_unique<MngUpdateCacheMsg>(MngUpdateCacheMsg(doc, m_dbService, m_cacheService));
			} else if (msgType.m_type == "mngDaemon_Version") {
				msg = std::make_unique<MngVersionMsg>(MngVersionMsg(doc));
			} else if (msgType.m_type == "mngScheduler_AddTask") {
				msg = std::make_unique<SchedulerAddTaskMsg>(SchedulerAddTaskMsg(doc, m_iSchedulerService));
			} else if (msgType.m_type == "mngScheduler_EditTask") {
				msg = std::make_unique<SchedulerEditTaskMsg>(SchedulerEditTaskMsg(doc, m_iSchedulerService));
			} else if (msgType.m_type == "mngScheduler_GetTask") {
				msg = std::make_unique<SchedulerGetTaskMsg>(SchedulerGetTaskMsg(doc, m_iSchedulerService));
			} else if (msgType.m_type == "mngScheduler_List") {
				msg = std::make_unique<SchedulerListMsg>(SchedulerListMsg(doc, m_iSchedulerService));
			} else if (msgType.m_type == "mngScheduler_RemoveAll") {
				msg = std::make_unique<SchedulerRemoveAllMsg>(SchedulerRemoveAllMsg(doc, m_iSchedulerService));
			} else if (msgType.m_type == "mngScheduler_RemoveTask") {
				msg = std::make_unique<SchedulerRemoveTaskMsg>(SchedulerRemoveTaskMsg(doc, m_iSchedulerService));
			} else if (msgType.m_type == "mngScheduler_StartTask") {
				msg = std::make_unique<SchedulerChangeTaskStateMsg>(SchedulerChangeTaskStateMsg(doc, m_iSchedulerService, true));
			} else if (msgType.m_type == "mngScheduler_StopTask") {
				msg = std::make_unique<SchedulerChangeTaskStateMsg>(SchedulerChangeTaskStateMsg(doc, m_iSchedulerService, false));
			} else {
				THROW_EXC_TRC_WAR(std::logic_error, "Unknown message type: " << msgType.m_type);
			}
			try {
				msg->handleMsg();
				msg->setStatus("ok", 0);
				msg->createResponse(respDoc);
				m_iMessagingSplitterService->sendMessage(messaging, std::move(respDoc));
			} catch (const std::exception &e) {
				msg->setErrorString(e.what());
				msg->setStatus("err", -1);
				Document errorDoc;
				msg->createResponse(errorDoc);
				m_iMessagingSplitterService->sendMessage(messaging, std::move(errorDoc));
			}
			TRC_FUNCTION_LEAVE("");
		}

		void handleSchedulerMsg(const rapidjson::Value &val) {
			(void)val;
			TRC_INFORMATION(std::endl << "Scheduled Exit ... " << std::endl);
			std::cout << std::endl << "Scheduled Exit ... " << std::endl;
			m_iLaunchService->exit();
		}

		void activate(const shape::Properties *props) {
			(void)props;
			TRC_FUNCTION_ENTER("");
			TRC_INFORMATION(std::endl
				<< "******************************" << std::endl
				<< "JsonMngApi instance activate" << std::endl
				<< "******************************"
			);
			m_iMessagingSplitterService->registerFilteredMsgHandler(
				m_filters,
				[&](const MessagingInstance &messaging, const IMessagingSplitterService::MsgType &msgType, rapidjson::Document doc) {
					handleMsg(messaging, msgType, std::move(doc));
				}
			);
			m_iSchedulerService->registerTaskHandler(
				"JsonMngApi",
				[&](const rapidjson::Value &val) {
					handleSchedulerMsg(val);
				}
			);
			TRC_FUNCTION_LEAVE("")
		}

		void modify(const shape::Properties *props) {
			(void)props;
		}

		void deactivate() {
			TRC_FUNCTION_ENTER("");
			TRC_INFORMATION(std::endl
				<< "******************************" << std::endl
				<< "JsonMngApi instance deactivate" << std::endl
				<< "******************************"
			);
			m_iMessagingSplitterService->unregisterFilteredMsgHandler(m_filters);
			m_iSchedulerService->unregisterTaskHandler("JsonMngApi");

			TRC_FUNCTION_LEAVE("")
		}

		void attachInterface(shape::ILaunchService *iface) {
			m_iLaunchService = iface;
		}

		void detachInterface(shape::ILaunchService *iface) {
			if (m_iLaunchService == iface) {
				m_iLaunchService = nullptr;
			}
		}

		void attachInterface(IIqrfDpaService *iface) {
			m_dpaService = iface;
		}

		void attachInterface(IIqrfDb *iface) {
			m_dbService = iface;
		}

		void detachInterface(IIqrfDb *iface) {
			if (m_dbService == iface) {
				m_dbService = nullptr;
			}
		}

		void detachInterface(IIqrfDpaService *iface) {
			if (m_dpaService == iface) {
				m_dpaService = nullptr;
			}
		}

		void attachInterface(ISchedulerService *iface) {
			m_iSchedulerService = iface;
		}

		void detachInterface(ISchedulerService *iface) {
			if (m_iSchedulerService == iface) {
				m_iSchedulerService = nullptr;
			}
		}

		void attachInterface(IJsCacheService *iface) {
			m_cacheService = iface;
		}

		void detachInterface(IJsCacheService *iface) {
			if (m_cacheService == iface) {
				m_cacheService = nullptr;
			}
		}

		void attachInterface(IUdpConnectorService *iface) {
			m_iUdpConnectorService = iface;
		}

		void detachInterface(IUdpConnectorService *iface) {
			if (m_iUdpConnectorService == iface) {
				m_iUdpConnectorService = nullptr;
			}
		}

		void attachInterface(IMessagingSplitterService *iface) {
			m_iMessagingSplitterService = iface;
		}

		void detachInterface(IMessagingSplitterService *iface) {
			if (m_iMessagingSplitterService == iface) {
				m_iMessagingSplitterService = nullptr;
			}
		}

		void attachInterface(shape::ITraceService *iface) {
			m_traceService = iface;
			shape::Tracer::get().addTracerService(iface);
		}

		void detachInterface(shape::ITraceService *iface) {
			m_traceService = nullptr;
			shape::Tracer::get().removeTracerService(iface);
		}
	};

	/////////////////////////
	JsonMngApi::JsonMngApi() {
		m_imp = shape_new Imp();
	}

	JsonMngApi::~JsonMngApi() {
		delete m_imp;
	}

	void JsonMngApi::activate(const shape::Properties *props) {
		m_imp->activate(props);
	}

	void JsonMngApi::deactivate() {
		m_imp->deactivate();
	}

	void JsonMngApi::modify(const shape::Properties *props) {
		m_imp->modify(props);
	}

	void JsonMngApi::attachInterface(shape::ILaunchService *iface) {
		m_imp->attachInterface(iface);
	}

	void JsonMngApi::detachInterface(shape::ILaunchService *iface) {
		m_imp->detachInterface(iface);
	}

	void JsonMngApi::attachInterface(IIqrfDb *iface) {
		m_imp->attachInterface(iface);
	}

	void JsonMngApi::detachInterface(IIqrfDb *iface) {
		m_imp->detachInterface(iface);
	}

	void JsonMngApi::attachInterface(IIqrfDpaService *iface) {
		m_imp->attachInterface(iface);
	}

	void JsonMngApi::detachInterface(IIqrfDpaService *iface) {
		m_imp->detachInterface(iface);
	}

	void JsonMngApi::attachInterface(ISchedulerService *iface) {
		m_imp->attachInterface(iface);
	}

	void JsonMngApi::detachInterface(ISchedulerService *iface) {
		m_imp->detachInterface(iface);
	}

	void JsonMngApi::attachInterface(IJsCacheService *iface) {
		m_imp->attachInterface(iface);
	}

	void JsonMngApi::detachInterface(IJsCacheService *iface) {
		m_imp->detachInterface(iface);
	}

	void JsonMngApi::attachInterface(IUdpConnectorService *iface) {
		m_imp->attachInterface(iface);
	}

	void JsonMngApi::detachInterface(IUdpConnectorService *iface) {
		m_imp->detachInterface(iface);
	}

	void JsonMngApi::attachInterface(IMessagingSplitterService *iface) {
		m_imp->attachInterface(iface);
	}

	void JsonMngApi::detachInterface(IMessagingSplitterService *iface) {
		m_imp->detachInterface(iface);
	}

	void JsonMngApi::attachInterface(shape::ITraceService *iface) {
		m_imp->attachInterface(iface);
	}

	void JsonMngApi::detachInterface(shape::ITraceService *iface) {
		m_imp->detachInterface(iface);
	}
}
