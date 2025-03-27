#pragma once

#include <set>

#include <models/light.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::Light;

namespace iqrf::db::repos {

class LightRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    std::unique_ptr<Light> get(const uint32_t id) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, deviceId
            FROM light
            WHERE id = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, id);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Light>(Light::fromResult(stmt));
    }

    std::vector<Light> getAll() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, deviceId
            FROM light;
            )"
        );
        std::vector<Light> vec;
        while (stmt.executeStep()) {
            vec.emplace_back(Light::fromResult(stmt));
        }
        return vec;
    }

    std::unique_ptr<Light> getByDeviceId(const uint32_t deviceId) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, deviceId
            FROM light
            WHERE deviceId = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, deviceId);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Light>(Light::fromResult(stmt));
    }

    uint32_t insert(Light &light) {
        SQLite::Statement stmt(*m_db,
            R"(
            INSERT INTO light (deviceId)
            VALUES (?);
            )"
        );
        stmt.bind(1, light.getDeviceId());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to insert new Light entity",
                    e.what()
                )
            );
        }
        return m_db->getLastInsertRowid();
    }

    void update(Light &light) {
        SQLite::Statement stmt(*m_db,
            R"(
            UPDATE light
            SET deviceId = ?
            WHERE id = ?;
            )"
        );
        stmt.bind(1, light.getDeviceId());
        stmt.bind(2, light.getId());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to update driver entity ID " + std::to_string(light.getId()),
                    e.what()
                )
            );
        }
    }

    void remove(const uint32_t id) {
        SQLite::Statement stmt(*m_db,
            R"(
            DELETE FROM light
            WHERE id = ?;
            )"
        );
        stmt.bind(1, id);
        stmt.exec();
    }

    void removeByDeviceId(const uint32_t deviceId) {
        SQLite::Statement stmt(*m_db,
            R"(
            DELETE FROM light
            WHERE deviceId = ?;
            )"
        );
        stmt.bind(1, deviceId);
        stmt.exec();
    }

    std::set<uint8_t> getAddresses() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT d.address
            FROM light as l
            INNER JOIN device as d ON d.id = l.deviceId;
            )"
        );
        std::set<uint8_t> addrs;
        while(stmt.executeStep()) {
            addrs.insert(static_cast<uint8_t>(stmt.getColumn(0).getUInt()));
        }
        return addrs;
    }
};

}
