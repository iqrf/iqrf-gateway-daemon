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

#define IIqrfChannelService_EXPORTS

#include "IqrfUart.h"
#include "AccessControl.h"
#include "rapidjson/pointer.h"
#include <mutex>
#include <thread>
#include <atomic>
#include <cstring>

#include "iqrf/connector/uart/UartConnector.h"

#ifndef SHAPE_PLATFORM_WINDOWS
#include <termios.h>
#endif

#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
#define TRC_CHANNEL 0

#include "IIqrfChannelService.h"
#include "Trace.h"

#include "iqrf__IqrfUart.hxx"

TRC_INIT_MODULE(iqrf::IqrfUart)

const unsigned SPI_REC_BUFFER_SIZE = 1024;

namespace iqrf {

	class IqrfUart::Imp
	{
	public:
		Imp()
			:m_accessControl(this)
		{
		}

		~Imp()
		{
		}

		void send(const std::basic_string<unsigned char>& message) {
			TRC_INFORMATION("Sending to IQRF UART: " << std::endl << MEM_HEX(message.data(), message.size()));
			try {
				m_connector->send(std::vector<uint8_t>(message.begin(), message.end()));
				m_accessControl.sniff(message);
			} catch (const std::exception &e) {
				TRC_WARNING("Failed to send data via connector: " << e.what());
				throw;
			}
		}

		bool enterProgrammingState() {
			TRC_FUNCTION_ENTER("");
			TRC_WARNING("Not implemented");
			TRC_FUNCTION_LEAVE("");
			return false;
		}

		IIqrfChannelService::UploadErrorCode upload(
			const UploadTarget target,
			const std::basic_string<uint8_t>& data,
			const uint16_t address
		)
		{
			TRC_FUNCTION_ENTER("");
			TRC_WARNING("Not implemented");
			//silence -Wunused-parameter
			(void)target;
			(void)data;
			(void)address;

			TRC_FUNCTION_LEAVE("");
			//return IIqrfChannelService::Accessor::UploadErrorCode::UPLOAD_NO_ERROR;
			return IIqrfChannelService::UploadErrorCode::UPLOAD_ERROR_NOT_SUPPORTED;
		}

		bool terminateProgrammingState() {
			TRC_INFORMATION("Terminating programming mode.");
			TRC_WARNING("Not implemented");
			return false;
		}

		void startListen() {
			m_runListenThread = true;
			m_listenThread = std::thread(&IqrfUart::Imp::listen, this);
		}

		IIqrfChannelService::State getState()
		{
			if(m_runListenThread) {
				return State::Ready;
			}
			return State::NotReady;
		}

		std::unique_ptr<IIqrfChannelService::Accessor> getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
		{
			return m_accessControl.getAccess(receiveFromFunc, access);
		}

		bool hasExclusiveAccess() const
		{
			return m_accessControl.hasExclusiveAccess();
		}

		IIqrfChannelService::osInfo getTrModuleInfo()
		{
			TRC_FUNCTION_ENTER("");
			TRC_WARNING("Reading TR module identification - not implemented.");

			IIqrfChannelService::osInfo myOsInfo;
			memset(&myOsInfo, 0, sizeof(myOsInfo));

			TRC_FUNCTION_LEAVE("");
			return myOsInfo;
		}

		void activate(const shape::Properties *props)
		{
			TRC_FUNCTION_ENTER("");
			TRC_INFORMATION(std::endl <<
				"******************************" << std::endl <<
				"IqrfUart instance activate" << std::endl <<
				"******************************"
			);

			modify(props);
		}

		void deactivate() {
			TRC_FUNCTION_ENTER("");

			TRC_DEBUG("joining udp listening thread");
			TRC_DEBUG("listening thread joined");

			delete m_connector;

			TRC_INFORMATION(std::endl <<
				"******************************" << std::endl <<
				"IqrfUart instance deactivate" << std::endl <<
				"******************************"
			);
			TRC_FUNCTION_LEAVE("")
		}

		void modify(const shape::Properties *props) {
			using namespace rapidjson;

			Document d;
			d.CopyFrom(props->getAsJson(), d.GetAllocator());

			m_interfaceName = Pointer("/IqrfInterface").Get(d)->GetString();
			m_baudRate = Pointer("/baudRate").GetWithDefault(d, UART_BAUD_RATE_DEFAULT).GetUint();

			auto powerEnablePin = Pointer("/powerEnableGpioPin").GetWithDefault(d, POWER_ENABLE_GPIO_DEFAULT).GetInt64();
			m_powerEnableGpio = getGpio(powerEnablePin);

			auto busEnablePin = Pointer("/busEnableGpioPin").GetWithDefault(d, BUS_ENABLE_GPIO_DEFAULT).GetInt64();
			m_busEnableGpio = getGpio(busEnablePin);

			auto pgmSwitchPin = Pointer("/pgmSwitchGpioPin").GetWithDefault(d, PGM_SWITCH_GPIO_DEFAULT).GetInt64();
			m_pgmSwitchGpio = getGpio(pgmSwitchPin);

			int64_t uartEnablePin = UART_ENABLE_GPIO_DEFAULT;
			int64_t spiEnablePin = SPI_ENABLE_GPIO_DEFAULT;
			int64_t i2cEnablePin = I2C_ENABLE_GPIO_DEFAULT;

			if (busEnablePin == -1) {
				uartEnablePin = Pointer("/uartEnableGpioPin").GetWithDefault(d, UART_ENABLE_GPIO_DEFAULT).GetInt64();
				spiEnablePin = Pointer("/spiEnableGpioPin").GetWithDefault(d, SPI_ENABLE_GPIO_DEFAULT).GetInt64();
				i2cEnablePin = Pointer("/i2cEnableGpioPin").GetWithDefault(d, I2C_ENABLE_GPIO_DEFAULT).GetInt64();
			}
			m_uartEnableGpio = getGpio(uartEnablePin);
			m_spiEnableGpio = getGpio(spiEnablePin);
			m_i2cEnableGpio = getGpio(i2cEnablePin);

			m_trReset = Pointer("/uartReset").Get(d)->GetBool();

			auto cfg = iqrf::connector::uart::UartConfig(
				m_interfaceName,
				m_baudRate,
				m_powerEnableGpio,
				m_busEnableGpio,
				m_pgmSwitchGpio,
				m_spiEnableGpio,
				m_uartEnableGpio,
				m_i2cEnableGpio,
				m_trReset
			);
			m_connector = new connector::uart::UartConnector(cfg);
		}

		void listen()
		{
			TRC_FUNCTION_ENTER("thread starts");

			try {
				while (m_runListenThread)
				{
					if (state == State::NotReady) {
						THROW_EXC_TRC_WAR(std::logic_error, "UART interface not ready to listen.");
					}

					auto recvBuffer = m_connector->receive();

					if (!recvBuffer.empty()) {
						TRC_INFORMATION("Received from IQRF UART: " << std::endl << MEM_HEX(recvBuffer.data(), recvBuffer.size()));
						m_accessControl.messageHandler(recvBuffer.data());
					}
				}
			}
			catch (const std::exception& e) {
				TRC_WARNING("Listening thread error: " << e.what());
				m_runListenThread = false;
			}
			TRC_WARNING("Listening thread stopped");
		}

	private:
		std::optional<iqrf::gpio::Gpio> getGpio(int pin) {
			if (pin < 0) {
				return std::nullopt;
			}
			try {
				auto cfg = iqrf::gpio::GpioConfig(pin);
				std::cout << "GPIO from pin " << std::to_string(pin) << ": chip - " << cfg.chip << ", line - " << std::to_string(cfg.line) << std::endl;
				return iqrf::gpio::Gpio(cfg);
			} catch (const std::exception &e) {
				std::cerr << "Cannot initialize GPIO: " << e.what() << std::endl;
				return std::nullopt;
			}
		}

		AccessControl<IqrfUart::Imp> m_accessControl;

		IIqrfChannelService::State state = State::Ready;
		std::atomic_bool m_runListenThread;
		std::thread m_listenThread;

		std::string m_interfaceName;
		int m_baudRate = 0;
		std::optional<gpio::Gpio> m_powerEnableGpio = std::nullopt;
		std::optional<gpio::Gpio> m_busEnableGpio = std::nullopt;
		std::optional<gpio::Gpio> m_pgmSwitchGpio = std::nullopt;
		std::optional<gpio::Gpio> m_uartEnableGpio = std::nullopt;
		std::optional<gpio::Gpio> m_spiEnableGpio = std::nullopt;
		std::optional<gpio::Gpio> m_i2cEnableGpio = std::nullopt;
		bool m_trReset = false;
		iqrf::connector::uart::UartConnector *m_connector = nullptr;
	};

	//////////////////////////////////////////////////
	IqrfUart::IqrfUart()
	{
		m_imp = shape_new Imp();
	}

	IqrfUart::~IqrfUart()
	{
		delete m_imp;
	}

	void IqrfUart::startListen()
	{
		return m_imp->startListen();
	}

	IIqrfChannelService::State IqrfUart::getState() const
	{
		return m_imp->getState();
	}

	std::unique_ptr<IIqrfChannelService::Accessor>  IqrfUart::getAccess(ReceiveFromFunc receiveFromFunc, AccesType access)
	{
		return m_imp->getAccess(receiveFromFunc, access);
	}

	bool IqrfUart::hasExclusiveAccess() const
	{
		return m_imp->hasExclusiveAccess();
	}

	void IqrfUart::activate(const shape::Properties *props)
	{
		m_imp->activate(props);
	}

	void IqrfUart::deactivate()
	{
		m_imp->deactivate();
	}

	void IqrfUart::modify(const shape::Properties *props)
	{
		m_imp->modify(props);
	}

	void IqrfUart::attachInterface(shape::ITraceService* iface)
	{
		shape::Tracer::get().addTracerService(iface);
	}

	void IqrfUart::detachInterface(shape::ITraceService* iface)
	{
		shape::Tracer::get().removeTracerService(iface);
	}
}
