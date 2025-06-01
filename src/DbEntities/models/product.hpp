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
#include <optional>
#include <set>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::models {

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
	 * Constructor without ID
	 * @param hwpid HWPID
	 * @param hwpidVersion HWPID version
	 * @param osBuild OS build
	 * @param osVersion OS version
	 * @param dpaVersion DPA version
	 * @param handlerUrl Handler URL
	 * @param handlerHash Handler hash
	 * @param customDriver Custom product driver
	 * @param packageId Package ID
	 * @param name Product name
	 */
	Product(const uint16_t hwpid, const uint16_t hwpidVersion, const uint16_t osBuild, const std::string& osVersion,
		const uint16_t dpaVersion, std::shared_ptr<std::string> handlerUrl = nullptr,
		std::shared_ptr<std::string> handlerHash = nullptr, std::shared_ptr<std::string> customDriver = nullptr,
		std::optional<uint32_t> packageId = std::nullopt, std::shared_ptr<std::string> name = nullptr)
		: hwpid(hwpid),
		  hwpidVersion(hwpidVersion),
		  osBuild(osBuild),
		  osVersion(osVersion),
		  dpaVersion(dpaVersion),
		  handlerUrl(handlerUrl),
		  handlerHash(handlerHash),
		  customDriver(customDriver),
		  packageId(packageId),
		  name(name) {};

	/**
	 * Full constructor
	 * @param id ID
	 * @param hwpid HWPID
	 * @param hwpidVersion HWPID version
	 * @param osBuild OS build
	 * @param osVersion OS version
	 * @param dpaVersion DPA version
	 * @param handlerUrl Handler URL
	 * @param handlerHash Handler hash
	 * @param customDriver Custom product driver
	 * @param packageId Package ID
	 * @param name Product name
	 */
	Product(const uint32_t id, const uint16_t hwpid, const uint16_t hwpidVersion, const uint16_t osBuild,
		const std::string& osVersion, const uint16_t dpaVersion, std::shared_ptr<std::string> handlerUrl = nullptr,
		std::shared_ptr<std::string> handlerHash = nullptr, std::shared_ptr<std::string> customDriver = nullptr,
		std::optional<uint32_t> packageId = std::nullopt, std::shared_ptr<std::string> name = nullptr)
		: id(id),
		  hwpid(hwpid),
		  hwpidVersion(hwpidVersion),
		  osBuild(osBuild),
		  osVersion(osVersion),
		  dpaVersion(dpaVersion),
		  handlerUrl(handlerUrl),
		  handlerHash(handlerHash),
		  customDriver(customDriver),
		  packageId(packageId),
		  name(name) {};

	/**
	 * Returns product ID
	 * @return Product ID
	 */
	uint32_t getId() const {
		return id;
	}

	/**
	 * Sets product ID
	 * @param id Product ID
	 */
	void setId(const uint32_t id) {
		this->id = id;
	}

	/**
	 * Returns product HWPID
	 * @return Product HWPID
	 */
	uint16_t getHwpid() const {
		return hwpid;
	}

	/**
	 * Sets product HWPID
	 * @param Product HWPID
	 */
	void setHwpid(const uint16_t hwpid) {
		this->hwpid = hwpid;
	}

	/**
	 * Returns product HWPID version
	 * @return Product HWPID version
	 */
	uint16_t getHwpidVersion() const {
		return hwpidVersion;
	}

	/**
	 * Sets product HWPID version
	 * @param hwpidVersion Product HWPID version
	 */
	void setHwpidVersion(const uint16_t hwpidVersion) {
		this->hwpidVersion = hwpidVersion;
	}

	/**
	 * Returns product OS build
	 * @return Product OS build
	 */
	uint16_t getOsBuild() const {
		return osBuild;
	}

	/**
	 * Sets product OS build
	 * @param osBuild Product OS build
	 */
	void setOsBuild(const uint16_t osBuild) {
		this->osBuild = osBuild;
	}

	/**
	 * Returns product OS version
	 * @return Product OS version
	 */
	const std::string& getOsVersion() const {
		return osVersion;
	}

	/**
	 * Sets product OS version
	 * @param osVersion Product OS version
	 */
	void setOsVersion(const std::string& osVersion) {
		this->osVersion = osVersion;
	}

	/**
	 * Returns product DPA version
	 * @return Product DPA version
	 */
	uint16_t getDpaVersion() const {
		return dpaVersion;
	}

	/**
	 * Sets product DPA version
	 * @param dpaVersion Product DPA version
	 */
	void setDpaVersion(const uint16_t dpaVersion) {
		this->dpaVersion = dpaVersion;
	}

	/**
	 * Returns product handler url
	 * @return Product handler url
	 */
	std::shared_ptr<std::string> getHandlerUrl() const {
		return handlerUrl;
	}

	/**
	 * Sets product hnadler url
	 * @param handlerUrl Product handler url
	 */
	void setHandlerUrl(std::shared_ptr<std::string> handlerUrl = nullptr) {
		this->handlerUrl = std::move(handlerUrl);
	}

	/**
	 * Returns product handler hash
	 * @return Product handler hash
	 */
	std::shared_ptr<std::string> getHandlerHash() const {
		return handlerHash;
	}

	/**
	 * Sets product handler hash
	 * @param handlerHash Product handler hash
	 */
	void setHandlerHash(std::shared_ptr<std::string> handlerHash = nullptr) {
		this->handlerHash = std::move(handlerHash);
	}

	/**
	 * Returns product custom driver
	 * @return Product custom driver
	 */
	std::shared_ptr<std::string> getCustomDriver() const {
		return customDriver;
	}

	/**
	 * Sets product custom driver
	 * @param customDriver Product custom driver
	 */
	void setCustomDriver(std::shared_ptr<std::string> customDriver = nullptr) {
		this->customDriver = std::move(customDriver);
	}

	/**
	 * Returns product package ID
	 * @return Product package ID
	 */
	std::optional<uint32_t> getPackageId() const {
		return packageId;
	}

	/**
	 * Sets product package ID
	 * @param packageId Product package ID
	 */
	void setPackageId(std::optional<uint32_t> packageId = std::nullopt) {
		this->packageId = packageId;
	}

	/**
	 * Returns product name
	 * @return Product name
	 */
	std::shared_ptr<std::string> getName() const {
		return name;
	}

	/**
	 * Sets product name
	 * @param name Product name
	 */
	void setName(std::shared_ptr<std::string> name = nullptr) {
		this->name = std::move(name);
	}

	/**
	 * Checks whether enumerated product is valid
	 */
	bool isValid() {
		return osBuild > 0 && dpaVersion > 0;
	}

	static Product fromResult(SQLite::Statement &stmt) {
		auto id = stmt.getColumn(0).getUInt();
		auto hwpid = static_cast<uint16_t>(stmt.getColumn(1).getUInt());
		auto hwpidVersion = static_cast<uint16_t>(stmt.getColumn(2).getUInt());
		auto osBuild = static_cast<uint16_t>(stmt.getColumn(3).getUInt());
		auto osVersion = stmt.getColumn(4).getString();
		auto dpaVersion = static_cast<uint16_t>(stmt.getColumn(5).getUInt());
		std::shared_ptr<std::string> handlerUrl = nullptr;
		if (!stmt.getColumn(6).isNull()) {
			handlerUrl = std::make_shared<std::string>(stmt.getColumn(6).getString());
		}
		std::shared_ptr<std::string> handlerHash = nullptr;
		if (!stmt.getColumn(7).isNull()) {
			handlerHash = std::make_shared<std::string>(stmt.getColumn(7).getString());
		}
		std::shared_ptr<std::string> customDriver = nullptr;
		if (!stmt.getColumn(8).isNull()) {
			customDriver = std::make_shared<std::string>(stmt.getColumn(8).getString());
		}
		std::optional<uint32_t> packageId = std::nullopt;
		if (!stmt.getColumn(9).isNull()) {
			packageId = stmt.getColumn(9).getUInt();
		}
		std::shared_ptr<std::string> name = nullptr;
		if (!stmt.getColumn(10).isNull()) {
			name = std::make_shared<std::string>(stmt.getColumn(10).getString());
		}
		return Product(id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash,
			customDriver, packageId, name);
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
	std::shared_ptr<std::string> handlerUrl;
	/// Product handler hash
	std::shared_ptr<std::string> handlerHash;
	/// Product customDriver
	std::shared_ptr<std::string> customDriver;
	/// Product package ID
	std::optional<uint32_t> packageId;
	/// Product name
	std::shared_ptr<std::string> name;
};

}
