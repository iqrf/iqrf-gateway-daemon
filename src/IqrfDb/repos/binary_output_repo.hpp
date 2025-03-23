#pragma once

#include "models/binary_output.hpp"

/**
 * Binary Output repository class
 */
class BinaryOutputRepo {
public:

    static std::unique_ptr<TBinaryOutput> get(unsigned long long id) {
        auto entity = TBinaryOutput::find(QVariant(id));
        if (!entity.has_value()) {
            return nullptr;
        }
        return std::make_unique<TBinaryOutput>(entity.value());
    }

    static unsigned long long create(
        unsigned char count,
        unsigned long long deviceId
    ) {
        return TBinaryOutput::create({
            {"count", count},
            {"device_id", deviceId}
        }).getId();
    }

    static void update(const TBinaryOutput& entity) {
        entity.save();
    }
};
