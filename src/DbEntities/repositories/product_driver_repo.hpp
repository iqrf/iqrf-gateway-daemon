#pragma once

#include <set>
#include <vector>

#include <models/driver.hpp>
#include <models/product_driver.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::ProductDriver;

namespace iqrf::db::repos {

class ProductDriverRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    std::vector<ProductDriver> getAll() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT productId, driverId
            FROM productDriver;
            )"
        );
        std::vector<ProductDriver> vector;
        while (stmt.executeStep()) {
            vector.emplace_back(ProductDriver::fromResult(stmt));
        }
        return vector;
    }

    uint32_t insert(ProductDriver &productDriver) {
        SQLite::Statement stmt(*m_db,
            R"(
            INSERT INTO productDriver (productId, driverId)
            VALUES (?, ?);
            )"
        );
        stmt.bind(1, productDriver.getProductId());
        stmt.bind(2, productDriver.getDriverId());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to insert new ProductDriver entity",
                    e.what()
                )
            );
        }
        return m_db->getLastInsertRowid();
    }

    void remove(const uint32_t productId, const uint32_t driverId) {
        SQLite::Statement stmt(*m_db,
            R"(
            DELETE FROM productDriver
            WHERE productId = ? AND driverId = ?;
            )"
        );
        stmt.bind(1, productId);
        stmt.bind(2, driverId);
        stmt.exec();
    }

    std::vector<Driver> getDrivers(const uint32_t productId) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT d.id, d.name, d.peripheralNumber, d.version, d.versionFlags, d.driver, d.driverHash
            FROM driver as d
            INNER JOIN productDriver as pd ON pd.driverId = d.id
            WHERE pd.productId = ?;
            )"
        );
        stmt.bind(1, productId);
        std::vector<Driver> vector;
        while (stmt.executeStep()) {
            vector.emplace_back(Driver::fromResult(stmt));
        }
        return vector;
    }

    std::set<uint32_t> getDriverIds(const uint32_t productId) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT driverId
            FROM productDriver
            WHERE productId = ?;
            )"
        );
        stmt.bind(1, productId);
        std::set<uint32_t> set;
        while (stmt.executeStep()) {
            set.insert(stmt.getColumn(0).getUInt());
        }
        return set;
    }

    std::map<uint32_t, std::set<uint32_t>> getProductsDriversMap() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT p.id, d.id
            FROM driver as d
            INNER JOIN productDriver as pd ON pd.driverId = d.id
            INNER JOIN product as p ON p.id = pd.productId;
            )"
        );
        std::map<uint32_t, std::set<uint32_t>> map;
        while (stmt.executeStep()) {
            auto productId = stmt.getColumn(0).getUInt();
            auto driverId = stmt.getColumn(0).getUInt();
            if (map.count(productId) == 0) {
                map.insert(
                    std::make_pair(
                        productId,
                        std::set<uint32_t>{driverId}
                    )
                );
            } else {
                map[productId].insert(driverId);
            }
        }
        return map;
    }
};

}
