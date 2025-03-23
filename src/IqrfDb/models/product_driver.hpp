#pragma once

#include <orm/tiny/model.hpp>

using Orm::Tiny::Model;

/**
 * Product driver entity class
 */
class TProductDriver final : public Model<TProductDriver> {
	friend Model;
    using Model::Model;

public:
	unsigned long long getId() {
		return this->getAttribute("id").toULongLong();
	}
private:
	/// Table name
	QString u_table{"product_drivers"};
	/// Disable default timestamps
	bool u_timestamps = false;
	/// Mass-assignable attributes
	inline static QStringList u_fillable {
		"product_id",
		"driver_id"
	};
};
