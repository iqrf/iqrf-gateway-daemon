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

#include <cstdint>
#include <string>
#include "BaseEntity.h"

namespace iqrf::db {

	/**
	 * IQRF DB Sensor entity
	 */
	class Sensor : public BaseEntity {
	public:
		/**
		 * Base constructor
		 */
		Sensor() = default;

		/**
		 * Full constructor
		 * @param type Sensor type
		 * @param name Sensor name
		 * @param shortname Sensor short name
		 * @param unit Sensor unit
		 * @param decimals Unit decimal places
		 * @param frc2b Implements FRC 2 bit command
		 * @param frc1B Implements FRC 1 byte command
		 * @param frc2B Implements FRC 2 byte command
		 * @param frc4B Implements FRC 4 byte command
		 */
		Sensor(const uint8_t &type, const std::string &name, const std::string &shortname, const std::string &unit,
			const uint8_t &decimals, bool frc2Bit, bool frc1Byte, bool frc2Byte, bool frc4Byte)
		{
			this->type = type;
			this->name = name;
			this->shortname = shortname;
			this->unit = unit;
			this->decimals = decimals;
			this->frc2Bit = frc2Bit;
			this->frc1Byte = frc1Byte;
			this->frc2Byte = frc2Byte;
			this->frc4Byte = frc4Byte;
		}

		/**
		 * Returns Sensor ID
		 * @return Sensor ID
		 */
		const uint32_t& getId() const {
			return this->id;
		}

		/**
		 * Sets Sensor ID
		 * @param id Sensor ID
		 */
		void setId(const uint32_t &id) {
			this->id = id;
		}

		/**
		 * Returns Sensor type
		 * @return Sensor type
		 */
		const uint8_t& getType() const {
			return this->type;
		}

		/**
		 * Sets Sensor Type
		 * @param type Sensor type
		 */
		void setType(const uint8_t &type) {
			this->type = type;
		}

		/**
		 * Returns Sensor name
		 * @return Sensor name
		 */
		const std::string& getName() const {
			return this->name;
		}

		/**
		 * Sets Sensor name
		 * @param name Sensor name
		 */
		void setName(const std::string &name) {
			this->name = name;
		}

		/**
		 * Returns Sensor short name
		 * @return Sensor short name
		 */
		const std::string& getShortname() const {
			return this->shortname;
		}

		/**
		 * Sets Sensor short name
		 * @param shortname Sensor short name
		 */
		void setShortname(const std::string &shortname) {
			this->shortname = shortname;
		}

		/**
		 * Returns Sensor unit
		 * @return Sensor unit
		 */
		const std::string& getUnit() const {
			return this->unit;
		}

		/**
		 * Sets Sensor unit
		 * @param unit Sensor unit
		 */
		void setUnit(const std::string &unit) {
			this->unit = unit;
		}

		/**
		 * Returns unit decimal places
		 * @return Unit decimal places
		 */
		const uint8_t& getDecimals() const {
			return this->decimals;
		}

		/**
		 * Sets unit decimal places
		 * @param decimals Unit decimal places
		 */
		void setDecimals(const uint8_t &decimals) {
			this->decimals = decimals;
		}

		/**
		 * Does sensor implement FRC 2 bit command?
		 * @return true if sensor implements FRC 2 bit command
		 */
		bool hasFrc2Bit() const {
			return this->frc2Bit;
		}

		/**
		 * Sets FRC 2 bit command
		 * @param frc2Bit FRC 2 bit command
		 */
		void setFrc2Bit(bool frc2Bit) {
			this->frc2Bit = frc2Bit;
		}

		/**
		 * Does sensor implement FRC 1 byte command?
		 * @return true if sensor implements FRC 1 byte command
		 */
		bool hasFrc1Byte() const {
			return this->frc1Byte;
		}

		/**
		 * Sets FRC 1 byte command
		 * @param frc1B FRC 1 byte command
		 */
		void setFrc1Byte(bool frc1Byte) {
			this->frc1Byte = frc1Byte;
		}

		/**
		 * Does sensor implement FRC 2 byte command?
		 * @return true if sensor implements FRC 2 byte command
		 */
		bool hasFrc2Byte() const {
			return this->frc2Byte;
		}

		/**
		 * Sets FRC 2 byte command
		 * @param frc2Byte FRC 2 byte command
		 */
		void setFrc2Byte(bool frc2Byte) {
			this->frc2Byte = frc2Byte;
		}

		/**
		 * Does sensor implement FRC 4 byte command?
		 * @return true if sensor implements FRC 4 byte command
		 */
		bool hasFrc4Byte() const {
			return this->frc4Byte;
		}

		/**
		 * Sets FRC 4 byte command
		 * @param frc4B FRC 4 byte command
		 */
		void setFrc4Byte(bool frc4Byte) {
			this->frc4Byte = frc4Byte;
		}
	private:
		/// Sensor ID
		uint32_t id;
		/// Sensor type
		uint8_t type;
		/// Sensor name
		std::string name;
		/// Sensor short name
		std::string shortname;
		/// Sensor unit
		std::string unit;
		/// Unit decimal places
		uint8_t decimals;
		/// 2 bit FRC command
		bool frc2Bit;
		/// 1 byte FRC command
		bool frc1Byte;
		/// 2 byte FRC command
		bool frc2Byte;
		/// 4 byte FRC command
		bool frc4Byte;
	};

}
