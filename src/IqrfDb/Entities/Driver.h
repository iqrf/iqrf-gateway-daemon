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

/**
 * IQRF DB driver entity
 */
class Driver {
public:
	/**
	 * Base constructor
	 */
	Driver() = default;

	/**
	 * Full constructor
	 * @param name Driver name
	 * @param peripheralNumber Driver peripheral number
	 * @param version Driver version
	 * @param versionFlags Driver version flags
	 * @param driver Driver code
	 * @param driverHash Driver hash
	 */
	Driver(const std::string &name, const int16_t &peripheralNumber, const double &version, const uint8_t &versionFlags, const std::string &driver, const std::string &driverHash);

	/**
	 * Returns driver ID
	 * @return Driver ID
	 */
	const uint32_t& getId() const;

	/**
	 * Sets driver ID
	 * @param id Driver ID
	 */
	void setId(const uint32_t &id);

	/**
	 * Returns driver name
	 * @return Driver name
	 */
	const std::string& getName() const;

	/**
	 * Sets driver name
	 * @param name Driver name
	 */
	void setName(const std::string &name);

	/**
	 * Returns driver peripheral number
	 * @return Driver peripheral number
	 */
	const int16_t& getPeripheralNumber() const;

	/**
	 * Sets driver peripheral number
	 * @param peripheralNumber Driver peripheral number
	 */
	void setPeripheralNumber(const int16_t &peripheralNumber);

	/**
	 * Returns driver version
	 * @return Driver version
	 */
	const double& getVersion() const;

	/**
	 * Sets driver version
	 * @param version Driver version
	 */
	void setVersion(const double &version);

	/**
	 * Returns driver version flags
	 * @return Driver version flags
	 */
	const uint8_t& getVersionFlags() const;

	/**
	 * Sets driver version flags
	 * @param versionFlags Driver version flags
	 */
	void setVersionFlags(const uint8_t &versionFlags);

	/**
	 * Returns driver code
	 * @return Driver code
	 */
	const std::string& getDriver() const;

	/**
	 * Sets driver code
	 * @param driver Driver code
	 */
	void setDriver(const std::string &driver);

	/**
	 * Returns driver hash
	 * @return Driver hash
	 */
	const std::string& getDriverHash() const;

	/**
	 * Sets driver hash
	 * @param driverHash Driver hash
	 */
	void setDriverHash(const std::string &driverHash);
private:
	/// Driver ID
	uint32_t id;
	/// Driver name
	std::string name;
	/// Driver peripheral number
	int16_t peripheralNumber;
	/// Driver version
	double version;
	/// Driver version flags
	uint8_t versionFlags;
	/// Driver content
	std::string driver;
	/// Driver hash
	std::string driverHash;
};
