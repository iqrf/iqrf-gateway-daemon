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
	 * @param notes Driver notes
	 * @param driver Driver code
	 */
	Driver(const std::string &name, const int16_t &peripheralNumber, const double &version, const uint8_t &versionFlags, const std::string &notes, const std::string &driver);

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
	 * Returns driver notes
	 * @return Driver notes
	 */
	const std::string& getNotes() const;

	/**
	 * Sets driver notes
	 * @param notes Driver notes
	 */
	void setNotes(const std::string &notes);

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
	/// Driver notes
	std::string notes;
	/// Driver content
	std::string driver;
};
