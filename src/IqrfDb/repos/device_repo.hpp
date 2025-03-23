#pragma once

#include "models/device.hpp"

/**
 * Device repository class
 */
class DeviceRepo {
public:

    static std::unique_ptr<TDevice> get(unsigned long long id) {
        auto device = TDevice::find(QVariant(id));
        if (!device.has_value()) {
            return nullptr;
        }
        return std::make_unique<TDevice>(device.value());
    }

    static std::unique_ptr<TDevice> getByAddress(const unsigned char addr) {
        auto device = TDevice::where("address", "=", addr);
        return nullptr;
    }

    static std::set<unsigned char> getDeviceAddresses() {
        std::set<unsigned char> addrs;
        auto devices = TDevice::select("address")
            ->orderBy("address")
            .get();
        return addrs;
    }
};
