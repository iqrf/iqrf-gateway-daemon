#pragma once

#include <set>
#include <vector>

#include <models/driver.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::Driver;

namespace iqrf::db::repos {

class DriverRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    std::unique_ptr<Driver> get(const uint32_t id) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, name, peripheralNumber, version, versionFlags, driver, driverHash
            FROM driver
            WHERE id = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, id);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Driver>(Driver::fromResult(stmt));
    }

    std::vector<Driver> getAll() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, name, peripheralNumber, version, versionFlags, driver, driverHash
            FROM driver;
            )"
        );
        std::vector<Driver> vec;
        while (stmt.executeStep()) {
            vec.emplace_back(Driver::fromResult(stmt));
        }
        return vec;
    }

    std::unique_ptr<Driver> getByPeripheralVersion(const int16_t peripheral, const double version) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, name, peripheralNumber, version, versionFlags, driver, driverHash
            FROM driver
            WHERE peripheralNumber = ? AND version = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, peripheral);
        stmt.bind(2, version);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Driver>(Driver::fromResult(stmt));
    }

    uint32_t insert(Driver &driver) {
        SQLite::Statement stmt(*m_db,
            R"(
            INSERT INTO driver (name, peripheralNumber, version, versionFlags, driver, driverHash)
            VALUES (?, ?, ?, ?, ?, ?)
            )"
        );
        stmt.bind(1, driver.getName());
        stmt.bind(2, driver.getPeripheralNumber());
        stmt.bind(3, driver.getVersion());
        stmt.bind(4, driver.getVersionFlags());
        stmt.bind(5, driver.getDriver());
        stmt.bind(6, driver.getDriverHash());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to insert new Driver entity",
                    e.what()
                )
            );
        }
        return m_db->getLastInsertRowid();
    }

    void update(Driver &driver) {
        SQLite::Statement stmt(*m_db,
            R"(
            UPDATE driver
            SET name = ?, versionFlags = ?, driver = ?, driverHash = ?
            WHERE id = ?;
            )"
        );
        stmt.bind(1, driver.getName());
        stmt.bind(2, driver.getVersionFlags());
        stmt.bind(3, driver.getDriver());
        stmt.bind(4, driver.getDriverHash());
        stmt.bind(5, driver.getId());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to update driver entity ID " + std::to_string(driver.getId()),
                    e.what()
                )
            );
        }
    }

    std::vector<Driver> getLatest() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT d1.id, d1.name, d1.peripheralNumber, d1.version, d1.versionFlags, d1.driver, d1.driverHash
            FROM driver as d1
            WHERE d1.version = (
                SELECT MAX(d2.version)
                FROM driver AS d2
                WHERE d2.peripheralNumber = d1.peripheralNumber
            )
            GROUP BY d1.peripheralNumber;
            )"
        );
        std::vector<Driver> vec;
        while (stmt.executeStep()) {
            vec.emplace_back(Driver::fromResult(stmt));
        }
        return vec;
    }
};

}
