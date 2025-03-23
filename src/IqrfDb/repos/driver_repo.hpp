#pragma once

#include "models/driver.hpp"

/**
 * Driver repository class
 */
class DriverRepo {
public:

    static std::unique_ptr<TDriver> get(unsigned long long id) {
        auto driver = TDriver::find(QVariant(id));
        if (!driver.has_value()) {
            return nullptr;
        }
        return std::make_unique<TDriver>(driver.value());
    }

    static std::unique_ptr<unsigned long long> getIdByPeripheralAndVersion(int periperal, float version) {
        auto result = TDriver::select("driver.id")
            ->where("peripheral_number", "=", periperal)
            .where("version", "=", version)
            .get();
    }

    static unsigned long long create(
        const std::string& name,
        const short peripheralNumber,
        const double version,
        const unsigned short versionFlags,
        const std::string& driver,
        const std::string& driverHash
    ) {
        return TDriver::create({
            {"name", name.c_str()},
            {"peripheral_number", peripheralNumber},
            {"version", version},
            {"version_flags", versionFlags},
            {"driver", driver.c_str()},
            {"driver_hash", driverHash.c_str()},
        }).getId();
    }
};
