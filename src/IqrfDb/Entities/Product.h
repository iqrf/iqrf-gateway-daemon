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
#include <set>
#include <string>

/**
 * IQRF DB product entity
 */
class Product {
public:

	/**
	 * Base constructor
	 */
	Product() = default;

	/**
	 * Constructor for product without handler and driver information
	 */
	Product(const uint16_t &hwpid, const uint16_t &hwpidVersion, const uint16_t &osBuild, const std::string &osVersion, const uint16_t &dpaVersion);

	/**
	 * Returns product ID
	 * @return Product ID
	 */
	const uint32_t& getId() const;

	/**
	 * Sets product ID
	 * @param id Product ID
	 */
	void setId(const uint32_t &id);

	/**
	 * Returns product HWPID
	 * @return Product HWPID
	 */
	const uint16_t& getHwpid() const;

	/**
	 * Sets product HWPID
	 * @param Product HWPID
	 */
	void setHwpid(const uint16_t &hwpid);

	/**
	 * Returns product HWPID version
	 * @return Product HWPID version
	 */
	const uint16_t& getHwpidVersion() const;

	/**
	 * Sets product HWPID version
	 * @param hwpidVersion Product HWPID version
	 */
	void setHwpidVersion(const uint16_t &hwpidVersion);

	/**
	 * Returns product OS build
	 * @return Product OS build
	 */
	const uint16_t& getOsBuild() const;

	/**
	 * Sets product OS build
	 * @param osBuild Product OS build
	 */
	void setOsBuild(const uint16_t &osBuild);

	/**
	 * Returns product OS version
	 * @return Product OS version
	 */
	const std::string& getOsVersion() const;

	/**
	 * Sets product OS version
	 * @param osVersion Product OS version
	 */
	void setOsVersion(const std::string &osVersion);

	/**
	 * Returns product DPA version
	 * @return Product DPA version
	 */
	const uint16_t& getDpaVersion() const;

	/**
	 * Sets product DPA version
	 * @param dpaVersion Product DPA version
	 */
	void setDpaVersion(const uint16_t &dpaVersion);

	/**
	 * Returns product handler url
	 * @return Product handler url
	 */
	std::shared_ptr<std::string> getHandlerUrl() const;

	/**
	 * Sets product hnadler url
	 * @param handlerUrl Product handler url
	 */
	void setHandlerUrl(std::shared_ptr<std::string> handlerUrl);

	/**
	 * Returns product handler hash
	 * @return Product handler hash
	 */
	std::shared_ptr<std::string> getHandlerHash() const;

	/**
	 * Sets product handler hash
	 * @param handlerHash Product handler hash
	 */
	void setHandlerHash(std::shared_ptr<std::string> handlerHash);

	/**
	 * Returns product custom driver
	 * @return Product custom driver
	 */
	std::shared_ptr<std::string> getCustomDriver() const;

	/**
	 * Sets product custom driver
	 * @param customDriver Product custom driver
	 */
	void setCustomDriver(std::shared_ptr<std::string> customDriver);

	/**
	 * Returns product package ID
	 * @return Product package ID
	 */
	std::shared_ptr<uint32_t> getPackageId() const;

	/**
	 * Sets product package ID
	 * @param packageId Product package ID
	 */
	void setPackageId(std::shared_ptr<uint32_t> packageId);

	/**
	 * Returns product name
	 * @return Product name
	 */
	std::shared_ptr<std::string> getName() const;

	/**
	 * Sets product name
	 * @param name Product name
	 */
	void setName(std::shared_ptr<std::string> name);

	/**
	 * Checks whether enumerated product is valid
	 */
	bool isValid();

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
	/// Product customDriver
	std::shared_ptr<std::string> customDriver = nullptr;
	/// Product package ID
	std::shared_ptr<uint32_t> packageId = nullptr;
	/// Product name
	std::shared_ptr<std::string> name = nullptr;
};
