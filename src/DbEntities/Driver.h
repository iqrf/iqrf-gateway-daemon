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
	 * IQRF DB driver entity
	 */
	class Driver : public BaseEntity {
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
		Driver(const std::string &name, const int16_t &peripheralNumber, const double &version, const uint8_t &versionFlags, const std::string &notes, const std::string &driver) {
			this->name = name;
			this->peripheralNumber = peripheralNumber;
			this->version = version;
			this->versionFlags = versionFlags;
			this->notes = notes;
			this->driver = driver;
		}

		/**
		 * Returns driver ID
		 * @return Driver ID
		 */
		const uint32_t& getId() const {
			return this->id;
		}

		/**
		 * Sets driver ID
		 * @param id Driver ID
		 */
		void setId(const uint32_t &id) {
			this->id = id;
		}

		/**
		 * Returns driver name
		 * @return Driver name
		 */
		const std::string& getName() const {
			return this->name;
		}

		/**
		 * Sets driver name
		 * @param name Driver name
		 */
		void setName(const std::string &name) {
			this->name = name;
		}

		/**
		 * Returns driver peripheral number
		 * @return Driver peripheral number
		 */
		const int16_t& getPeripheralNumber() const {
			return this->peripheralNumber;
		}

		/**
		 * Sets driver peripheral number
		 * @param peripheralNumber Driver peripheral number
		 */
		void setPeripheralNumber(const int16_t &peripheralNumber) {
			this->peripheralNumber = peripheralNumber;
		}

		/**
		 * Returns driver version
		 * @return Driver version
		 */
		const double& getVersion() const {
			return this->version;
		}

		/**
		 * Sets driver version
		 * @param version Driver version
		 */
		void setVersion(const double &version) {
			this->version = version;
		}

		/**
		 * Returns driver version flags
		 * @return Driver version flags
		 */
		const uint8_t& getVersionFlags() const {
			return this->versionFlags;
		}

		/**
		 * Sets driver version flags
		 * @param versionFlags Driver version flags
		 */
		void setVersionFlags(const uint8_t &versionFlags) {
			this->versionFlags = versionFlags;
		}

		/**
		 * Returns driver notes
		 * @return Driver notes
		 */
		const std::string& getNotes() const {
			return this->notes;
		}

		/**
		 * Sets driver notes
		 * @param notes Driver notes
		 */
		void setNotes(const std::string &notes) {
			this->notes = notes;
		}

		/**
		 * Returns driver code
		 * @return Driver code
		 */
		const std::string& getDriver() const {
			return this->driver;
		}

		/**
		 * Sets driver code
		 * @param driver Driver code
		 */
		void setDriver(const std::string &driver) {
			this->driver = driver;
		}
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

}
