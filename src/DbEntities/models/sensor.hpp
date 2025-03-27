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
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::models {

/**
 * IQRF DB Sensor entity
 */
class Sensor {
public:
	/**
	 * Base constructor
	 */
	Sensor() = default;

	/**
	 * Constructor without ID
	 * @param type Sensor type
	 * @param name Sensor name
	 * @param shortname Sensor short name
	 * @param unit Sensor unit
	 * @param decimals Unit decimal places
	 * @param frc2b Implements FRC 2 bit command
	 * @param frcB Implements FRC 1 byte command
	 * @param frc2B Implements FRC 2 byte command
	 * @param frc4B Implements FRC 4 byte command
	 */
	Sensor(const uint8_t type, const std::string& name, const std::string& shortname, const std::string& unit,
		const uint8_t decimals, bool frc2b, bool frcB, bool frc2B, bool frc4B)
		: type(type),
		  name(name),
		  shortname(shortname),
		  unit(unit),
		  decimals(decimals),
		  frc2b(frc2b),
		  frcB(frcB),
		  frc2B(frc2B),
		  frc4B(frc4B) {}

	/**
	 * Full constructor
	 * @param id ID
	 * @param type Sensor type
	 * @param name Sensor name
	 * @param shortname Sensor short name
	 * @param unit Sensor unit
	 * @param decimals Unit decimal places
	 * @param frc2b Implements FRC 2 bit command
	 * @param frcB Implements FRC 1 byte command
	 * @param frc2B Implements FRC 2 byte command
	 * @param frc4B Implements FRC 4 byte command
	 */
	Sensor(const uint32_t id, const uint8_t type, const std::string& name, const std::string& shortname,
		const std::string& unit, const uint8_t decimals, bool frc2b, bool frcB, bool frc2B, bool frc4B)
		: id(id),
		  type(type),
		  name(name),
		  shortname(shortname),
		  unit(unit),
		  decimals(decimals),
		  frc2b(frc2b),
		  frcB(frcB),
		  frc2B(frc2B),
		  frc4B(frc4B) {}

	/**
	 * Returns Sensor ID
	 * @return Sensor ID
	 */
	uint32_t getId() const {
		return id;
	}

	/**
	 * Sets Sensor ID
	 * @param id Sensor ID
	 */
	void setId(const uint32_t id) {
		this->id = id;
	}

	/**
	 * Returns Sensor type
	 * @return Sensor type
	 */
	uint8_t getType() const {
		return type;
	}

	/**
	 * Sets Sensor Type
	 * @param type Sensor type
	 */
	void setType(const uint8_t type) {
		this->type = type;
	}

	/**
	 * Returns Sensor name
	 * @return Sensor name
	 */
	const std::string& getName() const {
		return name;
	}

	/**
	 * Sets Sensor name
	 * @param name Sensor name
	 */
	void setName(const std::string& name) {
		this->name = name;
	}

	/**
	 * Returns Sensor short name
	 * @return Sensor short name
	 */
	const std::string& getShortname() const {
		return shortname;
	}

	/**
	 * Sets Sensor short name
	 * @param shortname Sensor short name
	 */
	void setShortname(const std::string& shortname) {
		this->shortname = shortname;
	}

	/**
	 * Returns Sensor unit
	 * @return Sensor unit
	 */
	const std::string& getUnit() const {
		return unit;
	}

	/**
	 * Sets Sensor unit
	 * @param unit Sensor unit
	 */
	void setUnit(const std::string& unit) {
		this->unit = unit;
	}

	/**
	 * Returns unit decimal places
	 * @return Unit decimal places
	 */
	uint8_t getDecimals() const {
		return decimals;
	}

	/**
	 * Sets unit decimal places
	 * @param decimals Unit decimal places
	 */
	void setDecimals(const uint8_t decimals) {
		this->decimals = decimals;
	}

	/**
	 * Does sensor implement FRC 2 bit command?
	 * @return true if sensor implements FRC 2 bit command
	 */
	bool hasFrc2Bit() const {
		return frc2b;
	}

	/**
	 * Sets FRC 2 bit command
	 * @param frc2b FRC 2 bit command
	 */
	void setFrc2Bit(bool frc2b) {
		this->frc2b = frc2b;
	}

	/**
	 * Does sensor implement FRC 1 byte command?
	 * @return true if sensor implements FRC 1 byte command
	 */
	bool hasFrcByte() const {
		return frcB;
	}

	/**
	 * Sets FRC 1 byte command
	 * @param frcB FRC 1 byte command
	 */
	void setFrcByte(bool frcB) {
		this->frcB = frcB;
	}

	/**
	 * Does sensor implement FRC 2 byte command?
	 * @return true if sensor implements FRC 2 byte command
	 */
	bool hasFrc2Byte() const {
		return frc2B;
	}

	/**
	 * Sets FRC 2 byte command
	 * @param frc2Byte FRC 2 byte command
	 */
	void setFrc2Byte(bool frc2B) {
		this->frc2B = frc2B;
	}

	/**
	 * Does sensor implement FRC 4 byte command?
	 * @return true if sensor implements FRC 4 byte command
	 */
	bool hasFrc4Byte() const {
		return frc4B;
	}

	/**
	 * Sets FRC 4 byte command
	 * @param frc4B FRC 4 byte command
	 */
	void setFrc4Byte(bool frc4B) {
		this->frc4B = frc4B;
	}

	/**
	 * Returns set of implemented frc commands
	 * @return Set of implemetned FRC commands
	 */
	std::set<uint8_t> getImplementedFrcs() {
		std::set<uint8_t> frcs;
		if (this->hasFrc2Bit()) {
			frcs.insert(0x10);
		}
		if (this->hasFrcByte()) {
			frcs.insert(0x90);
		}
		if (this->hasFrc2Byte()) {
			frcs.insert(0xE0);
		}
		if (this->hasFrc4Byte()) {
			frcs.insert(0xF9);
		}
		return frcs;
	}

	static Sensor fromResult(SQLite::Statement &stmt) {
		auto id = stmt.getColumn(0).getUInt();
		auto type = static_cast<uint8_t>(stmt.getColumn(1).getUInt());
		auto name = stmt.getColumn(2).getString();
		auto shortname = stmt.getColumn(3).getString();
		auto unit = stmt.getColumn(4).getString();
		auto decimals = static_cast<uint8_t>(stmt.getColumn(5).getUInt());
		auto frc2b = stmt.getColumn(6).getUInt() != 0;
		auto frcB = stmt.getColumn(7).getUInt() != 0;
		auto frc2B = stmt.getColumn(8).getUInt() != 0;
		auto frc4B = stmt.getColumn(9).getUInt() != 0;
		return Sensor(id, type, name, shortname, unit, decimals, frc2b, frcB, frc2B, frc4B);
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
	bool frc2b;
	/// 1 byte FRC command
	bool frcB;
	/// 2 byte FRC command
	bool frc2B;
	/// 4 byte FRC command
	bool frc4B;
};

}
