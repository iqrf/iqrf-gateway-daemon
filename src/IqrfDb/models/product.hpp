#pragma once

#include <orm/tiny/model.hpp>

using Orm::Tiny::Model;

/**
 * Product entity class
 */
class TProduct final : public Model<TProduct> {
	friend Model;
    using Model::Model;

public:
	unsigned long long getId() {
		return this->getAttribute("id").toULongLong();
	}

	std::shared_ptr<std::string> getName() {
		auto val = this->getAttribute("name");
		if (val.isNull()) {
			return nullptr;
		}
		return std::make_shared<std::string>(val.toString().toStdString());
	}

	unsigned short getHwpid() {
		return static_cast<unsigned short>(this->getAttribute("hwpid").toUInt());
	}

	unsigned short getHwpidVersion() {
		return static_cast<unsigned short>(this->getAttribute("hwpid_version").toUInt());
	}

	unsigned short getOsBuild() {
		return static_cast<unsigned short>(this->getAttribute("os_build").toUInt());
	}

	std::string getOsVersion() {
		return this->getAttribute("os_version").toString().toStdString();
	}

	unsigned short getDpaVersion() {
		return static_cast<unsigned short>(this->getAttribute("dpa_version").toUInt());
	}

	std::string getHandlerUrl() {
		return this->getAttribute("handler_url").toString().toStdString();
	}

	std::string getHandlerHash() {
		return this->getAttribute("handler_hash").toString().toStdString();
	}

	std::string getCustomDriver() {
		return this->getAttribute("custom_driver").toString().toStdString();
	}

	unsigned int getPackageId() {
		return this->getAttribute("package_id").toUInt();
	}
private:
	/// Table name
	QString u_table{"products"};
	/// Disable default timestamps
	bool u_timestamps = false;
	/// Mass-assignable attributes
	inline static QStringList u_fillable {
		"name",
		"hwpid",
		"hwpid_version",
		"os_build",
		"os_version",
		"dpa_version",
        "handler_url",
        "handler_hash",
        "custom_driver",
        "package_id"
	};
};
