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
#include <memory>
#include <set>
#include <string>
#include "BaseEntity.h"

namespace iqrf::db {

	/**
	 * IQRF DB product entity
	 */
	class Product : public BaseEntity {
	public:

		/**
		 * Base constructor
		 */
		Product() = default;

		/**
		 * Constructor for product without handler and driver information
		 */
		Product(const uint16_t &hwpid, const uint16_t &hwpidVersion, const uint16_t &osBuild, const std::string &osVersion, const uint16_t &dpaVersion) {
			this->hwpid = hwpid;
			this->hwpidVersion = hwpidVersion;
			this->osBuild = osBuild;
			this->osVersion = osVersion;
			this->dpaVersion = dpaVersion;
			this->standardEnumerated = false;
		}

		/**
		 * Returns product ID
		 * @return Product ID
		 */
		const uint32_t& getId() const {
			return this->id;
		}

		/**
		 * Sets product ID
		 * @param id Product ID
		 */
		void setId(const uint32_t &id) {
			this->id = id;
		}

		/**
		 * Returns product HWPID
		 * @return Product HWPID
		 */
		const uint16_t& getHwpid() const {
			return this->hwpid;
		}

		/**
		 * Sets product HWPID
		 * @param Product HWPID
		 */
		void setHwpid(const uint16_t &hwpid) {
			this->hwpid = hwpid;
		}

		/**
		 * Returns product HWPID version
		 * @return Product HWPID version
		 */
		const uint16_t& getHwpidVersion() const {
			return this->hwpidVersion;
		}

		/**
		 * Sets product HWPID version
		 * @param hwpidVersion Product HWPID version
		 */
		void setHwpidVersion(const uint16_t &hwpidVersion) {
			this->hwpidVersion = hwpidVersion;
		}

		/**
		 * Returns product OS build
		 * @return Product OS build
		 */
		const uint16_t& getOsBuild() const {
			return this->osBuild;
		}

		/**
		 * Sets product OS build
		 * @param osBuild Product OS build
		 */
		void setOsBuild(const uint16_t &osBuild) {
			this->osBuild = osBuild;
		}

		/**
		 * Returns product OS version
		 * @return Product OS version
		 */
		const std::string& getOsVersion() const {
			return this->osVersion;
		}

		/**
		 * Sets product OS version
		 * @param osVersion Product OS version
		 */
		void setOsVersion(const std::string &osVersion) {
			this->osVersion = osVersion;
		}

		/**
		 * Returns product DPA version
		 * @return Product DPA version
		 */
		const uint16_t& getDpaVersion() const {
			return this->dpaVersion;
		}

		/**
		 * Sets product DPA version
		 * @param dpaVersion Product DPA version
		 */
		void setDpaVersion(const uint16_t &dpaVersion) {
			this->dpaVersion = dpaVersion;
		}

		/**
		 * Returns product handler url
		 * @return Product handler url
		 */
		std::shared_ptr<std::string> getHandlerUrl() const {
			return this->handlerUrl;
		}

		/**
		 * Sets product hnadler url
		 * @param handlerUrl Product handler url
		 */
		void setHandlerUrl(std::shared_ptr<std::string> handlerUrl) {
			this->handlerUrl = std::move(handlerUrl);
		}

		/**
		 * Returns product handler hash
		 * @return Product handler hash
		 */
		std::shared_ptr<std::string> getHandlerHash() const {
			return this->handlerHash;
		}

		/**
		 * Sets product handler hash
		 * @param handlerHash Product handler hash
		 */
		void setHandlerHash(std::shared_ptr<std::string> handlerHash) {
			this->handlerHash = std::move(handlerHash);
		}

		/**
		 * Returns product notes
		 * @return Product notes
		 */
		std::shared_ptr<std::string> getNotes() const {
			return this->notes;
		}

		/**
		 * Sets product notes
		 * @param notes Product notes
		 */
		void setNotes(std::shared_ptr<std::string> notes) {
			this->notes = std::move(notes);
		}

		/**
		 * Returns product custom driver
		 * @return Product custom driver
		 */
		std::shared_ptr<std::string> getCustomDriver() const {
			return this->customDriver;
		}

		/**
		 * Sets product custom driver
		 * @param customDriver Product custom driver
		 */
		void setCustomDriver(std::shared_ptr<std::string> customDriver) {
			this->customDriver = std::move(customDriver);
		}

		/**
		 * Returns product package ID
		 * @return Product package ID
		 */
		const uint32_t& getPackageId() const {
			return this->packageId;
		}

		/**
		 * Sets product package ID
		 * @param packageId Product package ID
		 */
		void setPackageId(const uint32_t &packageId) {
			this->packageId = packageId;
		}

		/**
		 * Returns product standard enumerated status
		 * @return true if product standard is enumerated
		 */
		bool isStandardEnumerated() const {
			return this->standardEnumerated;
		}

		/**
		 * Sets product standard enumerated status
		 * @param standardEnumerated Product standard enumerated status
		 */
		void setStandardEnumerated(bool standardEnumerated) {
			this->standardEnumerated = standardEnumerated;
		}

		/**
		 * Checks whether enumerated product is valid
		 */
		bool isValid() {
			return (this->osBuild > 0) && (this->dpaVersion > 0);
		}

		/// Set of driver IDs
		std::set<uint32_t> drivers;
	private:
		/// Product ID
		uint32_t id;
		/// Product HWPID
		uint16_t hwpid;
		/// Product HWPID version
		uint16_t hwpidVersion;
		/// Product OS build
		uint16_t osBuild;
		/// Product OS version
		std::string osVersion;
		/// Product DPA version
		uint16_t dpaVersion;
		/// Product handler url
		std::shared_ptr<std::string> handlerUrl = nullptr;
		/// Product handler hash
		std::shared_ptr<std::string> handlerHash = nullptr;
		/// Product notes
		std::shared_ptr<std::string> notes = nullptr;
		/// Product customDriver
		std::shared_ptr<std::string> customDriver = nullptr;
		/// Product package ID
		uint32_t packageId;
		/// Indicates whether standards have been enumerated
		bool standardEnumerated;
	};

}
