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
#include <memory>

/**
 * IQRF DB device entity
 */
class Device {
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
	Device(const uint8_t &address, bool discovered, const uint32_t &mid, const uint8_t &vrn, const uint8_t &zone, std::shared_ptr<uint8_t> parent);

	/**
	 * Destructor
	 */
	~Device() = default;

	/**
	 * Returns device ID
	 * @return Device ID
	 */
	const uint32_t& getId() const;

	/**
	 * Sets device ID
	 * @param id Device ID
	 */
	void setId(const uint32_t &id);

	/**
	 * Returns device network address
	 * @return Device network address
	 */
	const uint8_t& getAddress() const;

	/**
	 * Sets device network address
	 * @param address Device network address
	 */
	void setAddress(const uint8_t &address);

	/**
	 * Returns device discovered status
	 * @returns true if device is discovered, false otherwise
	 */
	bool isDiscovered() const;

	/**
	 * Sets device discovered status
	 * @param discovered Device discovered status
	 */
	void setDiscovered(bool discovered);

	/**
	 * Returns device module ID
	 * @return Device module ID
	 */
	const uint32_t& getMid() const;

	/**
	 * Sets device module ID
	 * @param mid Device module ID
	 */
	void setMid(const uint32_t &mid);

	/**
	 * Returns device virtual routing number
	 * @return Device virtual routing number
	 */
	const uint8_t& getVrn() const;

	/**
	 * Sets device virtual routing number
	 * @param vrn Device virtual routing number
	 */
	void setVrn(const uint8_t &vrn);

	/**
	 * Returns device zone
	 * @return Device zone
	 */
	const uint8_t& getZone() const;

	/**
	 * Sets device zone
	 * @param zone Device zone
	 */
	void setZone(const uint8_t &zone);

	/**
	 * Returns device parent address
	 * @return Device parent address if discovered, otherwise null
	 */
	std::shared_ptr<uint8_t> getParent() const;

	/**
	 * Sets device parent address
	 * @param Device parent address
	 */
	void setParent(std::shared_ptr<uint8_t> parent);

	/**
	 * Returns device enumerated status
	 * @return true if device is enumerated, false otherwise
	 */
	bool isEnumerated() const;

	/**
	 * Sets device enumerated status
	 * @param enumerated Device enumerated status
	 */
	void setEnumerated(bool enumerated);

	/**
	 * Returns device product ID
	 * @return Device product ID
	 */
	const uint32_t& getProductId() const;

	/**
	 * Sets device product ID
	 * @param productId Device product ID
	 */
	void setProductId(const uint32_t &productId);

	/**
	 * Returns device metadata
	 * @return Device metadata pointer
	 */
	std::shared_ptr<std::string> getMetadata() const;

	/**
	 * Sets device metadata
	 * @param metadata Device metadata pointer
	 */
	void setMetadata(std::shared_ptr<std::string> metadata);

	/**
	 * Checks whether enumerated device is valid
	 */
	bool isValid();
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
	/// Device metadata
	std::shared_ptr<std::string> metadata = nullptr;
};
