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
	 * Returns product standard enumerated status
	 * @return true if product standard is enumerated
	 */
	bool isStandardEnumerated() const;

	/**
	 * Sets product standard enumerated status
	 * @param standardEnumerated Product standard enumerated status
	 */
	void setStandardEnumerated(bool standardEnumerated);

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
	/// Indicates whether standards have been enumerated
	bool standardEnumerated;
};
