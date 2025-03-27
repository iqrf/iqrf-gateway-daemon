#pragma once

#include <set>
#include <vector>

#include <models/product.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::Product;

namespace iqrf::db::repos {

class ProductRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    std::unique_ptr<Product> get(const uint32_t id) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
                packageId, name
            FROM product
            WHERE id = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, id);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Product>(Product::fromResult(stmt));
    }

    std::unique_ptr<Product> get(const uint16_t hwpid, const uint16_t hwpidVersion, const uint16_t osBuild,
        const uint16_t dpaVersion) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
                packageId, name
            FROM product
            WHERE hwpid = ? AND hwpidVersion = ? AND osBuild = ? AND dpaVersion = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, hwpid);
        stmt.bind(2, hwpidVersion);
        stmt.bind(3, osBuild);
        stmt.bind(4, dpaVersion);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Product>(Product::fromResult(stmt));
    }

    std::vector<Product> getAll() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
                packageId, name
            FROM product;
            )"
        );
        std::vector<Product> vec;
        while (stmt.executeStep()) {
            vec.emplace_back(Product::fromResult(stmt));
        }
        return vec;
    }

    uint32_t insert(Product &product) {
        SQLite::Statement stmt(*m_db,
            R"(
            INSERT INTO product (hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash,
                customDriver, packageId, name)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
            )"
        );
        stmt.bind(1, product.getHwpid());
        stmt.bind(2, product.getHwpidVersion());
        stmt.bind(3, product.getOsBuild());
        stmt.bind(4, product.getOsVersion());
        stmt.bind(5, product.getDpaVersion());
        if (product.getHandlerUrl() == nullptr) {
            stmt.bind(6);
        } else {
            stmt.bind(6, *product.getHandlerUrl());
        }
        if (product.getHandlerHash() == nullptr) {
            stmt.bind(7);
        } else {
            stmt.bind(7, *product.getHandlerHash());
        }
        if (product.getCustomDriver() == nullptr) {
            stmt.bind(8);
        } else {
            stmt.bind(8, *product.getCustomDriver());
        }
        if (product.getPackageId() == std::nullopt) {
            stmt.bind(9);
        } else {
            stmt.bind(9, product.getPackageId().value());
        }
        if (product.getName() == nullptr) {
            stmt.bind(10);
        } else {
            stmt.bind(10, *product.getName());
        }
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to insert new Product entity",
                    e.what()
                )
            );
        }
        return m_db->getLastInsertRowid();
    }

    void update(Product &product) {
        SQLite::Statement stmt(*m_db,
            R"(
            UPDATE product
            SET hwpid = ?, hwpidVersion = ?, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
                packageId, name
            WHERE id = ?;
            )"
        );
        stmt.bind(1, product.getHwpid());
        stmt.bind(2, product.getHwpidVersion());
        stmt.bind(3, product.getOsBuild());
        stmt.bind(4, product.getOsVersion());
        stmt.bind(5, product.getDpaVersion());
        if (product.getHandlerUrl() == nullptr) {
            stmt.bind(6);
        } else {
            stmt.bind(6, *product.getHandlerUrl());
        }
        if (product.getHandlerHash() == nullptr) {
            stmt.bind(7);
        } else {
            stmt.bind(7, *product.getHandlerHash());
        }
        if (product.getCustomDriver() == nullptr) {
            stmt.bind(8);
        } else {
            stmt.bind(8, *product.getCustomDriver());
        }
        if (product.getPackageId() == std::nullopt) {
            stmt.bind(9);
        } else {
            stmt.bind(9, product.getPackageId().value());
        }
        if (product.getName() == nullptr) {
            stmt.bind(10);
        } else {
            stmt.bind(10, *product.getName());
        }
        stmt.bind(11, product.getId());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to update product entity ID " + std::to_string(product.getId()),
                    e.what()
                )
            );
        }
    }

    std::optional<uint32_t> getCoordinatorProductId() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT p.id
            FROM product as p
            INNER JOIN device as d on d.productId = p.id
            LIMIT 1;
            )"
        );
        if (!stmt.executeStep()) {
            return std::nullopt;
        }
        return stmt.getColumn(0).getUInt();
    }

    std::optional<uint32_t> getNoncertifiedProductId(const uint16_t hwpid, const uint16_t hwpidVersion,
        const uint16_t osBuild, const uint16_t dpaVersion) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
                packageId, name
            FROM product
            WHERE hwpid = ? AND hwpidVersion = ? AND osBuild = ? AND dpaVersion = ? and packageId = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, hwpid);
        stmt.bind(2, hwpidVersion);
        stmt.bind(3, osBuild);
        stmt.bind(4, dpaVersion);
        stmt.bind(5);
        if (!stmt.executeStep()) {
            return std::nullopt;
        }
        return stmt.getColumn(0).getUInt();
    }

    std::shared_ptr<std::string> getCustomDriver(const uint32_t productId) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT customDriver
            FROM product
            WHERE id = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, productId);
        if (!stmt.executeStep() || stmt.getColumn(0).isNull()) {
            return nullptr;
        }
        return std::make_shared<std::string>(stmt.getColumn(0).getString());
    }
};

}
