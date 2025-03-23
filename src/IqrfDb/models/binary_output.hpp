#pragma once

#include <orm/tiny/model.hpp>

using Orm::Tiny::Model;

/**
 * Binary Output entity class
 */
class TBinaryOutput final : public Model<TBinaryOutput> {
	friend Model;
    using Model::Model;

public:
	unsigned long long getId() {
		return this->getAttribute("id").toULongLong();
	}
private:
	/// Table name
	QString u_table{"binary_outputs"};
	/// Disable default timestamps
	bool u_timestamps = false;
	/// Mass-assignable attributes
	inline static QStringList u_fillable {
        "count",
        "device_id"
	};
};
