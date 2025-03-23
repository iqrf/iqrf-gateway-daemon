#pragma once

#include "models/product.hpp"

/**
 * Product repository class
 */
class ProductRepo {
public:

    static std::unique_ptr<TProduct> get(unsigned long long id) {
        auto entity = TProduct::find(QVariant(id));
        if (!entity.has_value()) {
            return nullptr;
        }
        return std::make_unique<TProduct>(entity.value());
    }
};
