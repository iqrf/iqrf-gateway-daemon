#pragma once

#include <orm/tiny/model.hpp>

using Orm::Tiny::Model;

/**
 * Device entity class
 */
class TDevice final : public Model<TDevice> {
	friend Model;
    using Model::Model;

public:
	/**
	 * Get device ID
	 */
	unsigned long long getId() const {
		return this->getAttribute("id").toULongLong();
	}

	/**
	 * Get device address
	 */
	unsigned char getAddress() const {
		return static_cast<unsigned char>(this->getAttribute("address").toUInt());
	}

	bool getDiscovered() const {
		return this->getAttribute("discovered").toBool();
	}

	/**
	 * Get module ID
	 */
	unsigned int getMid() const {
		return this->getAttribute("mid").toUInt();
	}

	unsigned char getVrn() const {
		return static_cast<unsigned char>(this->getAttribute("vrn").toUInt());
	}

	unsigned char getZone() const {
		return static_cast<unsigned char>(this->getAttribute("zone").toUInt());
	}

	std::shared_ptr<unsigned char> getParent() const {
		return std::make_shared<unsigned char>(static_cast<unsigned char>(this->getAttribute("zone").toUInt()));
	}

	std::shared_ptr<std::string> getMetadata() const {
		return nullptr;
	}

	unsigned long long getProductId() const {
		return this->getAttribute("product_id").toULongLong();
	}
private:
	/// Table name
	QString u_table{"devices"};
	/// Disable default timestamps
	bool u_timestamps = false;
	/// Mass-assignable attributes
	inline static QStringList u_fillable {
        "address",
        "discovered",
        "mid",
        "vrn",
        "zone",
        "parent",
        "enumerated",
        "metadata",
        "product_id"
	};
};
