/**
 * Copyright 2015-2017 MICRORISC s.r.o.
 * Copyright 2017 IQRF Tech s.r.o.
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

#include "DpaTask.h"

DpaTask::DpaTask(const std::string& prfName, uint8_t prfNum) : m_prfName(prfName) {
	DpaMessage::DpaPacket_t& packet = m_request.DpaPacket();

	packet.DpaRequestPacket_t.PNUM = prfNum;
	packet.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

	m_request.SetLength(sizeof(TDpaIFaceHeader));
}

DpaTask::DpaTask(const std::string& prfName, uint8_t prfNum, uint16_t address, uint8_t command) : m_prfName(prfName) {
	DpaMessage::DpaPacket_t& packet = m_request.DpaPacket();

	packet.DpaRequestPacket_t.NADR = address;
	packet.DpaRequestPacket_t.PNUM = prfNum;
	packet.DpaRequestPacket_t.PCMD = command;
	packet.DpaRequestPacket_t.HWPID = HWPID_DoNotCheck;

	m_request.SetLength(sizeof(TDpaIFaceHeader));
}

DpaTask::~DpaTask() {}

uint16_t DpaTask::getAddress() const
{
	return m_request.DpaPacket().DpaRequestPacket_t.NADR;
}

void DpaTask::setAddress(uint16_t address)
{
	m_request.DpaPacket().DpaRequestPacket_t.NADR = address;
}

uint16_t DpaTask::getHwpid() const
{
	return m_request.DpaPacket().DpaRequestPacket_t.HWPID;
}

void DpaTask::setHwpid(uint16_t hwpid)
{
	m_request.DpaPacket().DpaRequestPacket_t.HWPID = hwpid;
}

uint8_t DpaTask::getPcmd() const
{
	return m_request.DpaPacket().DpaRequestPacket_t.PCMD;
}

void DpaTask::setPcmd(uint8_t command)
{
	m_request.DpaPacket().DpaRequestPacket_t.PCMD = command;
}

void DpaTask::handleConfirmation(const DpaMessage& confirmation)
{
	m_confirmation_ts = std::chrono::system_clock::now();
	m_confirmation = confirmation;
}

void DpaTask::handleResponse(const DpaMessage& response)
{
	m_response_ts = std::chrono::system_clock::now();
	m_response = response;

	parseResponse(m_response);
}

void DpaTask::timestampRequest(const std::chrono::time_point<std::chrono::system_clock>& ts)
{
	m_request_ts = ts;
}

void DpaTask::timestampConfirmation(const std::chrono::time_point<std::chrono::system_clock>& ts)
{
  m_confirmation_ts = ts;
}

void DpaTask::timestampResponse(const std::chrono::time_point<std::chrono::system_clock>& ts)
{
  m_response_ts = ts;
}
