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

#include "IDpaTransactionResult2.h"

/// iqrf namespace
namespace iqrf {

	class FakeAsyncTransactionResult : public IDpaTransactionResult2 {
	public:
		/**
		 * Constructor
		 * @param dpaMessage DPA message
		 */
		FakeAsyncTransactionResult(const DpaMessage& dpaMessage) : m_now(std::chrono::system_clock::now()) {
			DpaMessage::MessageType direction = dpaMessage.MessageDirection();
			if (direction != DpaMessage::MessageType::kRequest && direction != DpaMessage::MessageType::kResponse) {
				return;
			}
			if (direction == DpaMessage::MessageType::kRequest) {
				m_request = dpaMessage;
			}
			m_response = dpaMessage;
		}

		/**
		 * Destructor
		 */
		virtual ~FakeAsyncTransactionResult() {};

		/**
		 * Returns error code
		 * @return Error code
		 */
		int getErrorCode() const override { return STATUS_NO_ERROR; }

		/**
		 * Dummy method
		 */
		void overrideErrorCode(ErrorCode err) override { (void)err; }

		/**
		 * Returns error string
		 * @return Error string
		 */
		std::string getErrorString() const override { return "ok"; }

		/**
		 * Returns DPA message request object
		 * @return DPA message request object
		 */
		virtual const DpaMessage& getRequest() const override { return m_request; }

		/**
		 * Returns DPA message confirmation object
		 * @return DPA message confirmation object
		 */
		virtual const DpaMessage& getConfirmation() const override { return m_confirmation; }

		/**
		 * Returns DPA message response object
		 * @return DPA message response object
		 */
		virtual const DpaMessage& getResponse() const override { return m_response; }

		/**
		 * Returns DPA request timestamp
		 * @return DPA request timestamp
		 */
		virtual const std::chrono::time_point<std::chrono::system_clock>& getRequestTs() const override { return m_now; }

		/**
		 * Returns DPA confirmation timestamp
		 * @return DPA confirmation timestamp
		 */
		virtual const std::chrono::time_point<std::chrono::system_clock>& getConfirmationTs() const override { return m_now; }

		/**
		 * Returns DPA response timestamp
		 * @return DPA response timestamp
		 */
		virtual const std::chrono::time_point<std::chrono::system_clock>& getResponseTs() const override { return m_now; }

		/**
		 * Has DPA request been confirmed?
		 * @return Request confirmed status
		 */
		virtual bool isConfirmed() const override { return false; }

		/**
		 * Has DPA response been received?
		 * @return Respons received status
		 */
		virtual bool isResponded() const override { return false; }

	private:
		/// DPA request timestamp
		std::chrono::time_point<std::chrono::system_clock> m_now;
		/// DPA message confirmation object
		DpaMessage m_confirmation;
		/// DPA message request object
		DpaMessage m_request;
		/// DPA message response object
		DpaMessage m_response;
	};
}
