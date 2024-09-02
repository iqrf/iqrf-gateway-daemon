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

#include "JsRenderDuktape.h"

#include "iqrf__JsRenderDuktape.hxx"

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

TRC_INIT_MODULE(iqrf::JsRenderDuktape)

using namespace rapidjson;

namespace iqrf {

	JsRenderDuktape::JsRenderDuktape() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	JsRenderDuktape::~JsRenderDuktape() {
		TRC_FUNCTION_ENTER("");
		TRC_FUNCTION_LEAVE("");
	}

	void JsRenderDuktape::activate(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "JsRenderDuktape instance activate" << std::endl
			<< "******************************");
		modify(props);
		TRC_FUNCTION_LEAVE("");
	}

	void JsRenderDuktape::modify(const shape::Properties *props) {
		TRC_FUNCTION_ENTER("");
		(void)props;
		TRC_FUNCTION_LEAVE("");
	}

	void JsRenderDuktape::deactivate() {
		TRC_FUNCTION_ENTER("");
		TRC_INFORMATION(std::endl
			<< "******************************" << std::endl
			<< "JsRenderDuktape instance deactivate" << std::endl
			<< "******************************");
		TRC_FUNCTION_LEAVE("");
	}

	bool JsRenderDuktape::loadContextCode(int contextId, const std::string &js, const std::set<uint32_t> &driverIdSet) {
		TRC_FUNCTION_ENTER(PAR(contextId));
		bool retval = true;
		try {
			std::unique_lock<std::mutex> lck(m_contextMtx);
			auto found = m_contexts.find(contextId);
			if (found != m_contexts.end()) {
				m_contexts.erase(contextId);
			}
			auto pair = std::make_pair(contextId, std::shared_ptr<Context>(shape_new Context()));
			pair.second->loadCode(js);
			m_contexts.insert(pair);
			m_contextDriverMap[contextId] = driverIdSet;
		} catch (const std::exception &e) {
			CATCH_EXC_TRC_WAR(std::exception, e, "Failed to load JS code for context " << std::to_string(contextId));
			shape::Tracer::get().writeMsg((int)shape::TraceLevel::Warning, 33, TRC_MNAME, __FILE__, __LINE__, __FUNCTION__, js);
			retval = false;
		}
		TRC_FUNCTION_LEAVE("");
		return retval;
	}

	void JsRenderDuktape::mapAddressToContext(int address, int contextId) {
		TRC_FUNCTION_ENTER(PAR(address) << PAR(contextId));
		std::unique_lock<std::mutex> lck(m_contextMtx);
		m_addressContextMap[address] = contextId;
		TRC_FUNCTION_LEAVE("");
	}

	std::set<uint32_t> JsRenderDuktape::getDriverIdSet(int contextId) const {
		std::unique_lock<std::mutex> lck(m_contextMtx);
		auto found = m_contextDriverMap.find(contextId);
		if (found != m_contextDriverMap.end()) {
			return found->second;
		}
		return std::set<uint32_t>();
	}

	void JsRenderDuktape::callContext(int address, int hwpid, const std::string &fname, const std::string &params, std::string &ret) {
		TRC_FUNCTION_ENTER(PAR(address) << PAR(hwpid) << PAR(fname));
		std::unique_lock<std::mutex> lck(m_contextMtx);

		bool addrContextUsed = true;
		std::shared_ptr<Context> ctx;
		try {
			ctx = findAddressContext(address);
			if (ctx == nullptr) {
				addrContextUsed = false;
				ctx = findHwpidContext(hwpid);
			}
		} catch (const std::logic_error &e) {
			CATCH_EXC_TRC_WAR(std::logic_error, e, e.what());
			THROW_EXC_TRC_WAR(std::logic_error, "Cannot find any usable context: " << PAR(address) << PAR(hwpid));
		}
		if (address == 0 && addrContextUsed) {
			bool driverError = false;
			try {
				ctx->callFunction(fname, params, ret);
			} catch (const PeripheralException &e) {
				driverError = true;
			} catch (const PeripheralCommandException &e) {
				driverError = true;
			}
			if (driverError) {
				TRC_DEBUG("Addr 0 context missing peripheral or command, retrying with provisional context.");
				int contextId = HWPID_DEFAULT_MAPPING;
				auto found = m_contexts.find(contextId);
				if (found == m_contexts.end()) {
					THROW_EXC_TRC_WAR(std::logic_error, "Default hwpid context not found for addr 0 fallback context.");
				}
				ctx = found->second;
				ctx->callFunction(fname, params, ret);
			}
		} else {
			ctx->callFunction(fname, params, ret);
		}
		TRC_FUNCTION_LEAVE("");
	}

	std::shared_ptr<Context> JsRenderDuktape::findAddressContext(int address) {
		auto addrContext = m_addressContextMap.find(address);
		if (addrContext == m_addressContextMap.end()) {
			return nullptr;
		}
		int contextId = addrContext->second;
		auto context = m_contexts.find(contextId);
		if (context == m_contexts.end()) {
			THROW_EXC_TRC_WAR(std::logic_error, "Cannot find JS context for address: " << PAR(address) << PAR(contextId));
		}
		TRC_DEBUG("Found address context: " << PAR(address) << PAR(contextId));
		return context->second;
	}

	std::shared_ptr<Context> JsRenderDuktape::findHwpidContext(int hwpid) {
		uint16_t uhwpid = (uint16_t)hwpid;
		int contextId = HWPID_MAPPING_SPACE - (int)uhwpid;
		auto context = m_contexts.find(contextId);
		if (context == m_contexts.end()) {
			contextId = HWPID_DEFAULT_MAPPING;
			context = m_contexts.find(contextId);
		} else {
			TRC_DEBUG("Using provisional hwpid context: " << PAR(uhwpid) << PAR(contextId));
		}
		if (context == m_contexts.end()) {
			THROW_EXC_TRC_WAR(std::logic_error, "Default hwpid context not found.");
		} else {
			TRC_DEBUG("Using default provisional hwpid context: " << PAR(uhwpid) << PAR(contextId));
		}
		return context->second;
	}

	std::shared_ptr<int> JsRenderDuktape::getDeviceAddrProductId(int address) const {
		auto result = m_addressContextMap.find(address);
		if (result == m_addressContextMap.end()) {
			return nullptr;
		}
		return std::make_shared<int>(result->second);
	}

	void JsRenderDuktape::clearContexts() {
		TRC_FUNCTION_ENTER("");
		std::unique_lock<std::mutex> lck(m_contextMtx);
		m_contexts.clear();
		m_addressContextMap.clear();
		m_contextDriverMap.clear();
		TRC_FUNCTION_LEAVE("");
	}

	void JsRenderDuktape::attachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().addTracerService(iface);
	}

	void JsRenderDuktape::detachInterface(shape::ITraceService *iface) {
		shape::Tracer::get().removeTracerService(iface);
	}
}
