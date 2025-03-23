#pragma once

#include <orm/tiny/model.hpp>

using Orm::Tiny::Model;

/**
 * Light entity class
 */
class TLight final : public Model<TLight> {
	friend Model;
    using Model::Model;

public:
	unsigned long long getId() {
		return this->getAttribute("id").toULongLong();
	}
private:
	/// Table name
	QString u_table{"lights"};
	/// Disable default timestamps
	bool u_timestamps = false;
	/// Mass-assignable attributes
	inline static QStringList u_fillable {
        "device_id"
	};
};
