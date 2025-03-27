#pragma once

#include <set>

#include <models/binary_output.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::BinaryOutput;

namespace iqrf::db::repos {

class BinaryOutputRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    std::unique_ptr<BinaryOutput> get(const uint32_t id) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, deviceId, count
            FROM bo
            WHERE id = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, id);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<BinaryOutput>(BinaryOutput::fromResult(stmt));
    }

    std::vector<BinaryOutput> getAll() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, deviceId, count
            FROM bo;
            )"
        );
        std::vector<BinaryOutput> vec;
        while (stmt.executeStep()) {
            vec.emplace_back(BinaryOutput::fromResult(stmt));
        }
        return vec;
    }

    std::unique_ptr<BinaryOutput> getByDeviceId(const uint32_t deviceId) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, deviceId, count
            FROM bo
            WHERE deviceId = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, deviceId);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<BinaryOutput>(BinaryOutput::fromResult(stmt));
    }

    uint32_t insert(BinaryOutput &binaryOutput) {
        SQLite::Statement stmt(*m_db,
            R"(
            INSERT INTO bo (deviceId, count)
            VALUES (?, ?);
            )"
        );
        stmt.bind(1, binaryOutput.getDeviceId());
        stmt.bind(2, binaryOutput.getCount());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to insert new BinaryOutput entity",
                    e.what()
                )
            );
        }
        return m_db->getLastInsertRowid();
    }

    void update(BinaryOutput &binaryOutput) {
        SQLite::Statement stmt(*m_db,
            R"(
            UPDATE bo
            SET deviceId = ?, count = ?
            WHERE id = ?;
            )"
        );
        stmt.bind(1, binaryOutput.getDeviceId());
        stmt.bind(2, binaryOutput.getCount());
        stmt.bind(3, binaryOutput.getId());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to update BinaryOutput entity ID " + std::to_string(binaryOutput.getId()),
                    e.what()
                )
            );
        }
    }

    void remove(const uint32_t id) {
        SQLite::Statement stmt(*m_db,
            R"(
            DELETE FROM bo
            WHERE id = ?;
            )"
        );
        stmt.bind(1, id);
        stmt.exec();
    }

    void removeByDeviceId(const uint32_t deviceId) {
        SQLite::Statement stmt(*m_db,
            R"(
            DELETE FROM bo
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
            FROM bo as b
            INNER JOIN device as d ON d.id = b.deviceId;
            )"
        );
        std::set<uint8_t> addrs;
        while(stmt.executeStep()) {
            addrs.insert(static_cast<uint8_t>(stmt.getColumn(0).getUInt()));
        }
        return addrs;
    }

    std::map<uint8_t, uint8_t> getAddressCountMap() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT d.address, b.count
            FROM bo as b
            INNER JOIN device as d ON d.id = b.deviceId;
            )"
        );
        std::map<uint8_t, uint8_t> map;
        while(stmt.executeStep()) {
            map.insert(
                std::make_pair(
                    static_cast<uint8_t>(stmt.getColumn(0).getUInt()),
                    static_cast<uint8_t>(stmt.getColumn(1).getUInt())
                )
            );
        }
        return map;
    }
};

}
