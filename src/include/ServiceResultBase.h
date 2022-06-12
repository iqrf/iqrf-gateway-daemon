/**
 * Copyright 2015-2021 IQRF Tech s.r.o.
 * Copyright 2019-2021 MICRORISC s.r.o.
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

#include "HexStringCoversion.h"
#include "IDpaTransaction2.h"
#include "IDpaTransactionResult2.h"
#include "rapidjson/document.h"
#include "rapidjson/pointer.h"

#include <list>

using namespace rapidjson;

/// iqrf namespace
namespace iqrf {
	/// Base service result class
	class ServiceResultBase {
	public:
		/**
		 * Sets message type
		 * @param mType Message type
		 */
		void setMessageType(const std::string &mType) {
			m_mType = mType;
		}

		/**
		 * Sets message ID
		 * @param msgId Message ID
		 */
		void setMessageId(const std::string &msgId) {
			m_msgId = msgId;
		}

		/**
		 * Sets response verbosity
		 * @param verbose Verbose response
		 */
		void setVerbose(bool verbose) {
			m_verbose = verbose;
		}

		/**
		 * Sets status code and string
		 * @param status Status code
		 * @param statusStr Status string
		 */
		void setStatus(const int status, const std::string &statusStr) {
			m_status = status;
			m_statusStr = statusStr;
		}

		/**
		 * Stores transaction result
		 * @param transactionResult Transaction result
		 */
		void addTransactionResult(std::unique_ptr<IDpaTransactionResult2> &transactionResult) {
			if (transactionResult) {
				m_transactionResults.push_back(std::move(transactionResult));
			}
		}

		/**
		 * Populates response document with request metadata
		 * @param response response document
		 */
		void setResponseMetadata(Document &response) {
			Pointer("/mType").Set(response, m_mType);
			Pointer("/data/msgId").Set(response, m_msgId);
		}

		/**
		 * Populates response document with transactions and codes
		 * @param response response document
		 */
		void createResponse(Document &response) {
			if (m_verbose) {
				Value array(kArrayType);
				Document::AllocatorType &allocator = response.GetAllocator();

				std::list<std::unique_ptr<IDpaTransactionResult2>>::iterator itr;
				for (itr = m_transactionResults.begin(); itr != m_transactionResults.end(); ++itr) {
					std::unique_ptr<IDpaTransactionResult2> result = std::move(*itr);
					Value object(kObjectType);
					object.AddMember(
						"request",
						encodeBinary(result->getRequest().DpaPacket().Buffer, result->getRequest().GetLength()),
						allocator
					);
					object.AddMember(
						"requestTs",
						encodeTimestamp(result->getRequestTs()),
						allocator
					);
					object.AddMember(
						"confirmation",
						encodeBinary(result->getConfirmation().DpaPacket().Buffer, result->getConfirmation().GetLength()),
						allocator
					);
					object.AddMember(
						"confirmationTs",
						encodeTimestamp(result->getConfirmationTs()),
						allocator
					);
					object.AddMember(
						"response",
						encodeBinary(result->getResponse().DpaPacket().Buffer, result->getResponse().GetLength()),
						allocator
					);
					object.AddMember(
						"responseTs",
						encodeTimestamp(result->getResponseTs()),
						allocator
					);
					array.PushBack(object, allocator);
				}
				Pointer("/data/raw").Set(response, array);
			}

			// Status
			Pointer("/data/status").Set(response, m_status);
			Pointer("/data/statusStr").Set(response, m_statusStr);
		}

		/**
		 * Populates response document with error response
		 * @param response Response document
		 */
		void createErrorResponse(Document &response) {
			// Default parameters
			Pointer("/mType").Set(response, m_mType);
			Pointer("/data/msgId").Set(response, m_msgId);

			// Status
			Pointer("/data/status").Set(response, m_status);
			Pointer("/data/statusStr").Set(response, m_statusStr);
		}
	protected:
		/// Message type
		std::string m_mType;
		/// Message ID
		std::string m_msgId;
		/// Verbose response
		bool m_verbose = false;
		/// Status code
		int m_status = 0;
		/// Status string
		std::string m_statusStr = "ok";
	private:
		/// List of DPA transactions performed during the service
		std::list<std::unique_ptr<IDpaTransactionResult2>> m_transactionResults;
	};
}
