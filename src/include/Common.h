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

#include <iomanip>
#include <string>
#include <sstream>

/// IQRF namespace
namespace iqrf {
	/// Common auxiliary utilities
	namespace common {
		/// Device auxiliary utilities
		namespace device {

			/// MCU type bitmask
			const uint8_t mcuTypeMask = 0x07;

			/// map of TR series (D) modules
			const std::map<uint8_t, std::string> trSeriesD = {
				{0, "52Dx"}, {1, "58_RJx"}, {2, "72Dx"}, {3, "53Dx"},
				{4, "78Dx"}, {8, "54Dx"}, {9, "55Dx"}, {10, "56Dx"},
				{11, "76Dx"}, {12, "77Dx"}, {13, "75Dx"}
			};

			/// map of TR series (G) modules
			const std::map<uint8_t, std::string> trSeriesG = {
				{2, "72Gx"}, {11, "76Gx"}
			};

			/**
			 * MCU type enum
			 */
			enum McuType {
				PIC16LF1938 = 4,
				PIC16LF18877
			};

			/**
			 * Converts module ID to hexadecimal string representation
			 * @param mid Module ID
			 * @return Module ID string
			 */
			std::string midString(const uint32_t &mid) {
				std::ostringstream oss;
				oss << std::uppercase << std::hex << std::setw(sizeof(uint32_t)*2) << std::setfill('0') << mid;
				return oss.str();
			}

			/**
			 * Reads module TR series and converts to string representation
			 * @param trMcu TR/MCU type
			 */
			std::string trSeriesString(const uint8_t &trMcu) {
				const uint8_t mcuType = trMcu & mcuTypeMask;
				const uint8_t trSeries = trMcu >> 4;
				std::ostringstream oss;
				oss << "(DC)TR-";
				if (mcuType == PIC16LF1938) {
					oss << (trSeriesD.count(trSeries) == 1) ? trSeriesD.at(trSeries) : "???";
				} else if (mcuType == PIC16LF18877) {
					oss << (trSeriesG.count(trSeries) == 1) ? trSeriesG.at(trSeries) : "???";
				} else {
					oss << "???";
				}
				return oss.str();
			}

			/**
			 * Reads module MCU type and converts to string representation
			 * @param trMcu TR/MCU type
			 */
			std::string mcuTypeString(const uint8_t &trMcu) {
				switch (trMcu & mcuTypeMask) {
					case PIC16LF1938:
						return "PIC16LF1938";
					case PIC16LF18877:
						return "PIC16LF18877";
					default:
						break;
				}
				return "unknown";
			}

			/**
			 * Converts OS build word to a hexadecimal string representation
			 * @param osBuild OS build word
			 * @return OS build string
			 */
			std::string osBuildString(const uint16_t &osBuild) {
				std::ostringstream oss;
				oss << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << osBuild;
				return oss.str();
			}

			/**
			 * Converts OS version byte to a string representation
			 * @param osVersion OS version byte
			 * @param trMcu TR/MCU type
			 * @return OS version string
			 */
			std::string osVersionString(const uint8_t &osVersion, const uint8_t &trMcu) {
				std::ostringstream oss;
				oss << std::hex << (osVersion >> 4) << '.' << std::setw(2) << std::setfill('0') << (osVersion & 0xf);
				switch (trMcu & mcuTypeMask) {
					case PIC16LF1938:
						oss << 'D';
						break;
					case PIC16LF18877:
						oss << 'G';
						break;
					default:
						break;
				}
				return oss.str();
			}

			/**
			 * Extracts DPA major version from DPA word
			 * @param dpaVersion DPA version word
			 * @return DPA major version
			 */
			uint8_t dpaVersionMajor(const uint16_t &dpaVersion) {
				return ((dpaVersion >> 8) & 0xFF);
			}

			/**
			 * Extracts DPA minor version from DPA word
			 * @param dpaVersion DPA version word
			 * @return DPA minor version
			 */
			uint8_t dpaVersionMinor(const uint16_t &dpaVersion) {
				return (dpaVersion & 0xFF);
			}

			/**
			 * Converts DPA version word to a human-readable string representation
			 * @param dpaVersion DPA version word
			 * @return DPA version string
			 */
			std::string dpaVersionString(const uint16_t &dpaVersion) {
				std::ostringstream oss;
				oss << std::uppercase << std::hex;
				oss << std::setw(1) << ((dpaVersion & 0x3FFF) >> 8) << '.' << std::setw(2) << (dpaVersion & 0xFF);
				return oss.str();
			}

			/**
			 * Converts DPA version word to a hexadecimal string reperesentation
			 * @param dpaVersion DPA version word
			 * @return DPA version string
			 */
			std::string dpaVersionHexaString(const uint16_t &dpaVersion) {
				std::ostringstream oss;
				oss << std::uppercase << std::setw(4) << std::setfill('0') << std::hex << dpaVersion;
				return oss.str();
			}

			/**
			 * Get DEMO flag from DPA version
			 * @param dpaVersion DPA version word
			 * @return true DPA demo, false otherwise
			 */
			bool demoFlag(const uint16_t &dpaVersion) {
				return (dpaVersion & 0x8000);
			}

			/**
			 * Checks if STD mode is supported. Negate value for LP mode support.
			 * @param flags Peripheral enumeration flags
			 * @return true if STD mode is supported, false if LP mode is supported
			 */
			bool stdModeSupported(const uint8_t &flags) {
				return (flags & 0x01);
			}

			/**
			 * Checks if STD+LP network is running.
			 * @param flags Peripheral enumeration flags
			 * @return true if STD+LP network is running, STD network otherwise
			 */
			bool stdLpNetwork(const uint8_t &flags) {
				return (flags & 0x04);
			}
		}
	}
}
