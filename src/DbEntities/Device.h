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
#include "BaseEntity.h"

namespace iqrf::db {

	/**
	 * IQRF DB device entity
	 */
	class Device : public BaseEntity {
	public:
		/**
		 * Base constructor
		 */
		Device() = default;

		/**
		 * Constructor for non-enumerated devices
		 * @param address Device network address
		 * @param discovered Indicates whether device is discovered
		 * @param mid Device module ID
		 * @param vrn Device virtual routing number
		 * @param zone Device zone
		 * @param parent Device parent
		 */
		Device(const uint8_t &address, bool discovered, const uint32_t &mid, const uint8_t &vrn, const uint8_t &zone, std::shared_ptr<uint8_t> parent) {
			this->address = address;
			this->discovered = discovered;
			this->mid = mid;
			this->vrn = vrn;
			this->zone = zone;
			this->parent = parent;
			this->enumerated = false;
			this->productId = 0;
		}

		/**
		 * Destructor
		 */
		~Device() = default;

		/**
		 * Returns device ID
		 * @return Device ID
		 */
		const uint32_t& getId() const {
			return this->id;
		}

		/**
		 * Sets device ID
		 * @param id Device ID
		 */
		void setId(const uint32_t &id) {
			this->id = id;
		}

		/**
		 * Returns device network address
		 * @return Device network address
		 */
		const uint8_t& getAddress() const {
			return this->address;
		}

		/**
		 * Sets device network address
		 * @param address Device network address
		 */
		void setAddress(const uint8_t &address) {
			this->address = address;
		}

		/**
		 * Returns device discovered status
		 * @returns true if device is discovered, false otherwise
		 */
		bool isDiscovered() const {
			return this->discovered;
		}

		/**
		 * Sets device discovered status
		 * @param discovered Device discovered status
		 */
		void setDiscovered(bool discovered) {
			this->discovered = discovered;
		}

		/**
		 * Returns device module ID
		 * @return Device module ID
		 */
		const uint32_t& getMid() const {
			return this->mid;
		}

		/**
		 * Sets device module ID
		 * @param mid Device module ID
		 */
		void setMid(const uint32_t &mid) {
			this->mid = mid;
		}

		/**
		 * Returns device virtual routing number
		 * @return Device virtual routing number
		 */
		const uint8_t& getVrn() const {
			return this->vrn;
		}

		/**
		 * Sets device virtual routing number
		 * @param vrn Device virtual routing number
		 */
		void setVrn(const uint8_t &vrn) {
			this->vrn = vrn;
		}

		/**
		 * Returns device zone
		 * @return Device zone
		 */
		const uint8_t& getZone() const {
			return this->zone;
		}

		/**
		 * Sets device zone
		 * @param zone Device zone
		 */
		void setZone(const uint8_t &zone) {
			this->zone = zone;
		}

		/**
		 * Returns device parent address
		 * @return Device parent address if discovered, otherwise null
		 */
		std::shared_ptr<uint8_t> getParent() const {
			return this->parent;
		}

		/**
		 * Sets device parent address
		 * @param Device parent address
		 */
		void setParent(std::shared_ptr<uint8_t> parent) {
			this->parent = std::move(parent);
		}

		/**
		 * Returns device enumerated status
		 * @return true if device is enumerated, false otherwise
		 */
		bool isEnumerated() const {
			return this->enumerated;
		}

		/**
		 * Sets device enumerated status
		 * @param enumerated Device enumerated status
		 */
		void setEnumerated(bool enumerated) {
			this->enumerated = enumerated;
		}

		/**
		 * Returns device product ID
		 * @return Device product ID
		 */
		const uint32_t& getProductId() const {
			return this->productId;
		}

		/**
		 * Sets device product ID
		 * @param productId Device product ID
		 */
		void setProductId(const uint32_t &productId) {
			this->productId = productId;
		}

		/**
		 * Returns user specified device name
		 * @return User specified device name
		 */
		std::shared_ptr<std::string> getName() const {
			return this->name;
		}

		/**
		 * Sets user specified device name
		 * @param name User specified device name
		 */
		void setName(std::shared_ptr<std::string> name) {
			this->name = std::move(name);
		}

		/**
		 * Returns user specified device location
		 * @return User specified device location
		 */
		std::shared_ptr<std::string> getLocation() const {
			return this->location;
		}

		/**
		 * Sets user specified device location
		 * @param location User specified device location
		 */
		void setLocation(std::shared_ptr<std::string> location) {
			this->location = std::move(location);
		}

		/**
		 * Returns device metadata
		 * @return Device metadata pointer
		 */
		std::shared_ptr<std::string> getMetadata() const {
			return this->metadata;
		}

		/**
		 * Sets device metadata
		 * @param metadata Device metadata pointer
		 */
		void setMetadata(std::shared_ptr<std::string> metadata) {
			this->metadata = std::move(metadata);
		}

		/**
		 * Checks whether enumerated device is valid
		 */
		bool isValid() {
			return this->mid > 0;
		}
	private:
		/// Device ID
		uint32_t id;
		/// Device address in network
		uint8_t address;
		/// Indicates whether device is discovered
		bool discovered;
		/// Module ID
		uint32_t mid;
		/// Device virtual routing number
		uint8_t vrn;
		/// Device zone
		uint8_t zone;
		/// Device parent
		std::shared_ptr<uint8_t> parent = nullptr;
		/// Indicates whether device is enumerated
		bool enumerated;
		/// Product ID
		uint32_t productId;
		/// User specified device name
		std::shared_ptr<std::string> name = nullptr;
		/// User specified device location
		std::shared_ptr<std::string> location = nullptr;
		/// Device metadata
		std::shared_ptr<std::string> metadata = nullptr;
	};

}
