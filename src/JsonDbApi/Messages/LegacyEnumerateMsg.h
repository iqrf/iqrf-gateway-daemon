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
#pragma once

#include "BaseMsg.h"

namespace iqrf {

    /**
     * Legacy Enumerate request message
     */
    class LegacyEnumerateMsg : public BaseMsg {
    public:
        /**
		 * Delete default constructor
		 */
		LegacyEnumerateMsg() = delete;

		/**
		 * Constructor
		 * @param doc Request document
		 */
        LegacyEnumerateMsg(const rapidjson::Document &doc) : BaseMsg(doc) {
            command = Pointer("/data/req/command").Get(doc)->GetString();
        }
        
        /**
         * Handle enumerate request
         * @param dbService IQRF DB service
		 */
        void handleMsg(IIqrfDb *dbService) override {
            if (command != "now" && command != "start") {
                throw std::invalid_argument("Command " + command + " is no longer supported.");
            }
            if (dbService->isRunning()) {
                throw std::runtime_error("Network enumeration already running.");
            }
            auto parameters = IIqrfDb::EnumParams{true, true};
            dbService->enumerate(parameters);
        }

		/**
		 * Populates response document with enumeration progress
		 * @param doc Response document
		 */
		void createResponsePayload(Document &doc) override {
			Document::AllocatorType &allocator = doc.GetAllocator();
			Pointer("/data/rsp/command").Set(doc, command, allocator);
			BaseMsg::createResponsePayload(doc);
		}

        /**
		 * Populates response document with enumeration error
		 * @param doc Response document
		 */
		void createErrorResponsePayload(Document &doc) {
			Pointer("/mType").Set(doc, getMType());
			Pointer("/data/msgId").Set(doc, getMsgId());
			BaseMsg::createResponsePayload(doc);
			if (getVerbose()) {
				Pointer("/data/insId").Set(doc, getInsId());
				Pointer("/data/statusStr").Set(doc, getStatusStr());
			}
			Pointer("/data/status").Set(doc, getStatus());
		}
    private:
        /// Enumeration command
        std::string command;
    };
}
