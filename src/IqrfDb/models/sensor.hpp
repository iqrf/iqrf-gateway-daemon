#pragma once

#include <orm/tiny/model.hpp>

using Orm::Tiny::Model;

/**
 * Sensor entity class
 */
class TSensor final : public Model<TSensor> {
	friend Model;
    using Model::Model;

public:
	unsigned long long getId() {
		return this->getAttribute("id").toULongLong();
	}
private:
	/// Table name
	QString u_table{"sensors"};
	/// Disable default timestamps
	bool u_timestamps = false;
	/// Mass-assignable attributes
	inline static QStringList u_fillable {
        "type",
        "name",
        "short_name",
        "unit",
        "decimals",
        "frc_2bit",
        "frc_byte",
        "frc_2byte",
        "frc_4byte"
	};
};
