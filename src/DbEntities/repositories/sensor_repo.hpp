#pragma once

#include <set>

#include <models/sensor.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::Sensor;

namespace iqrf::db::repos {

class SensorRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    std::unique_ptr<Sensor> get(const uint32_t id) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, type, name, shortname, unit, decimals, frc2bit, frc1Byte, frc2Byte, frc4Byte
            FROM sensor
            WHERE id = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, id);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Sensor>(Sensor::fromResult(stmt));
    }

    std::unique_ptr<Sensor> getByTypeName(const uint8_t type, const std::string& name) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, type, name, shortname, unit, decimals, frc2bit, frc1Byte, frc2Byte, frc4Byte
            FROM sensor
            WHERE type = ? AND name = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, type);
        stmt.bind(2, name);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Sensor>(Sensor::fromResult(stmt));
    }

    std::unique_ptr<Sensor> getByAddressIndexType(const uint8_t address, const uint8_t index, const uint8_t type) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT s.id, s.type, s.name, s.shortname, s.unit, s.decimals, s.frc2bit, s.frc1Byte, s.frc2Byte, s.frc4Byte
            FROM sensor as s
            INNER JOIN deviceSensor as ds
            ON ds.sensorId = s.id
            WHERE ds.address = ? and ds.globalIndex = ? and ds.type = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, address);
        stmt.bind(2, index);
        stmt.bind(3, type);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Sensor>(Sensor::fromResult(stmt));
    }

    uint32_t insert(Sensor &sensor) {
        SQLite::Statement stmt(*m_db,
            R"(
            INSERT INTO sensor (type, name, shortname, unit, decimals, frc2bit, frc1Byte, frc2Byte, frc4Byte)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
            )"
        );
        stmt.bind(1, sensor.getType());
        stmt.bind(2, sensor.getName());
        stmt.bind(3, sensor.getShortname());
        stmt.bind(4, sensor.getUnit());
        stmt.bind(5, sensor.getDecimals());
        stmt.bind(6, sensor.hasFrc2Bit());
        stmt.bind(7, sensor.hasFrcByte());
        stmt.bind(8, sensor.hasFrc2Byte());
        stmt.bind(9, sensor.hasFrc4Byte());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to insert new Sensor entity",
                    e.what()
                )
            );
        }
        return m_db->getLastInsertRowid();
    }

    std::map<uint8_t, Sensor> getDeviceSensorIndexMap(const uint8_t address) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT s.id, s.type, s.name, s.shortname, s.unit, s.decimals, s.frc2bit, s.frc1Byte, s.frc2Byte, s.frc4Byte
            FROM sensor as s
            INNER JOIN deviceSensor as ds
            ON ds.sensorId = s.id
            WHERE ds.address = ?
            ORDER BY ds.globalIndex;
            )"
        );
        stmt.bind(1, address);
        std::map<uint8_t, Sensor> map;
        size_t i = 0;
        while (stmt.executeStep()) {
            map.insert(
                std::make_pair(
                    i++,
                    Sensor::fromResult(stmt)
                )
            );
        }
        return map;
    }

    std::map<uint8_t, uint32_t> getDeviceSensorIdIndexMap(const uint8_t address) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT ds.globalIndex, s.id
            FROM sensor as s
            INNER JOIN deviceSensor as ds
            ON ds.sensorId = s.id
            WHERE ds.address = ?
            ORDER BY ds.globalIndex;
            )"
        );
        stmt.bind(1, address);
        std::map<uint8_t, uint32_t> map;
        while (stmt.executeStep()) {
            map.insert(
                std::make_pair(
                    static_cast<uint8_t>(stmt.getColumn(0).getUInt()),
                    stmt.getColumn(1).getUInt()
                )
            );
        }
        return map;
    }
};

}
