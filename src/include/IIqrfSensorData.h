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
#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>

namespace iqrf {

	//FRC command to return 2 - bits sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_2BITS = 0x10;
    //FRC command to return 1-byte wide sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_1BYTE = 0x90;
    //FRC command to return 2-bytes wide sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_2BYTES = 0xE0;
    //FRC command to return 4-bytes wide sensor data of the supporting sensor types.
    const int STD_SENSOR_FRC_4BYTES = 0xF9;

	/// @class IIqrfSensorData
	/// @brief IQRF Sensor data interface
	class IIqrfSensorData {
	public:
		/**
		 * Destructor
		 */
		virtual ~IIqrfSensorData() {};

		virtual bool readInProgress() = 0;
		virtual void registerReadingCallback(const std::string &instanceId, std::function<void(bool)> callback) = 0;
    virtual void unregisterReadingCallback(const std::string &messagingId) = 0;

		/**
		 * Get sensor FRC command from sensor type
		 * @param sensorType Sensor type (code)
		 * @return uint8_t FRC command code
		 */
		static uint8_t getSensorFrcCommand(const uint8_t &sensorType) {
			if (sensorType >= 0x01 && sensorType <= 0x1D) {
				return STD_SENSOR_FRC_2BYTES;
			} else if (sensorType >= 0x80 && sensorType <= 0x85) {
				return STD_SENSOR_FRC_1BYTE;
			} else if (sensorType >= 0xA0 && sensorType <= 0xA7) {
				return STD_SENSOR_FRC_4BYTES;
			}
			throw std::domain_error("Sensor type " + std::to_string(sensorType) + " not supported.");
		}

		/**
		 * Get node count per FRC command
		 * @param command FRC command
		 * @return uint8_t Node count
		 */
		static uint8_t getMaxDevicesPerCommand(const uint8_t &command) {
			switch (command) {
				case STD_SENSOR_FRC_2BITS:
					return 240;
				case STD_SENSOR_FRC_1BYTE:
					return 63;
				case STD_SENSOR_FRC_2BYTES:
					return 31;
				case STD_SENSOR_FRC_4BYTES:
				default:
					return 15;
			}
		}
	};
}
