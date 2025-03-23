#pragma once

#include <orm/tiny/model.hpp>

using Orm::Tiny::Model;

/**
 * Driver entity class
 */
class TDriver final : public Model<TDriver> {
	friend Model;
    using Model::Model;

public:
	unsigned long long getId() const {
		return this->getAttribute("id").toULongLong();
	}

	std::string getName() const {
		return this->getAttribute("name").toString().toStdString();
	}

	int getPeripheralNumber() const {
		return this->getAttribute("peripheral_number").toInt();
	}

	float getVersion() const {
		return this->getAttribute("version").toFloat();
	}

	unsigned int getVersionFlags() const {
		return this->getAttribute("version_flags").toFloat();
	}

	std::string getDriver() const {
		return this->getAttribute("driver").toString().toStdString();
	}

	void setDriver(const std::string& driver) {
		this->setAttribute("driver", driver.c_str());
	}

	std::string getDriverHash() const {
		return this->getAttribute("driver_hash").toString().toStdString();
	}

	void setDriverHash(const std::string& driverHash) {
		this->setAttribute("driver_hash", driverHash.c_str());
	}
private:
	/// Table name
	QString u_table{"drivers"};
	/// Disable default timestamps
	bool u_timestamps = false;
	/// Mass-assignable attributes
	inline static QStringList u_fillable {
		"name",
		"peripheral_number",
		"version",
		"version_flags",
		"driver",
		"driver_hash"
	};
};
