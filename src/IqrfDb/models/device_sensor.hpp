#pragma once

#include <orm/tiny/model.hpp>

using Orm::Tiny::Model;

/**
 * Device Sensor entity class
 */
class TDeviceSensor final : public Model<TDeviceSensor> {
	friend Model;
    using Model::Model;

public:
	unsigned long long getId() {
		return this->getAttribute("id").toULongLong();
	}
private:
	/// Table name
	QString u_table{"device_sensors"};
	/// Disable default timestamps
	bool u_timestamps = false;
	/// Mass-assignable attributes
	inline static QStringList u_fillable {
        "global_index",
        "type_index",
        "last_updated",
        "last_value",
        "metadata",
        "device_id",
        "sensor_id"
	};
};
