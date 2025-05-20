#pragma once

#include <set>
#include <vector>

#include "EmbedNode.h"
#include <models/device.hpp>
#include <models/product.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::Device;
using iqrf::db::models::Product;

namespace iqrf::db::repos {

class DeviceRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    std::unique_ptr<Device> get(const uint32_t id) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, address, discovered, mid, vrn, zone, parent, enumerated, productId, metadata
            FROM device
            WHERE id = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, id);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Device>(Device::fromResult(stmt));
    }

    std::vector<Device> getAll() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, address, discovered, mid, vrn, zone, parent, enumerated, productId, metadata
            FROM device
            ORDER BY address;
            )"
        );
        std::vector<Device> vec;
        while (stmt.executeStep()) {
            vec.emplace_back(Device::fromResult(stmt));
        }
        return vec;
    }

    std::unique_ptr<Device> getByAddress(const uint8_t addr) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, address, discovered, mid, vrn, zone, parent, enumerated, productId, metadata
            FROM device
            WHERE address = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, addr);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Device>(Device::fromResult(stmt));
    }

    std::unique_ptr<Device> getByMid(const uint32_t mid) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, address, discovered, mid, vrn, zone, parent, enumerated, productId, metadata
            FROM device
            WHERE mid = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, mid);
        if (!stmt.executeStep()) {
            return nullptr;
        }
        return std::make_unique<Device>(Device::fromResult(stmt));
    }

    uint32_t insert(Device &device) {
        SQLite::Statement stmt(*m_db,
            R"(
            INSERT INTO device (address, discovered, mid, vrn, zone, parent, enumerated, productId, metadata)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
            )"
        );
        stmt.bind(1, device.getAddress());
        stmt.bind(2, device.isDiscovered());
        stmt.bind(3, device.getMid());
        stmt.bind(4, device.getVrn());
        stmt.bind(5, device.getZone());
        if (device.getParent() == std::nullopt) {
            stmt.bind(6);
        } else {
            stmt.bind(6, device.getParent().value());
        }
        stmt.bind(7, device.isEnumerated());
        stmt.bind(8, device.getProductId());
        if (device.getMetadata() == nullptr) {
            stmt.bind(9);
        } else {
            stmt.bind(9, *device.getMetadata());
        }
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to insert new Device entity",
                    e.what()
                )
            );
        }
        return m_db->getLastInsertRowid();
    }

    void update(Device &device) {
        SQLite::Statement stmt(*m_db,
            R"(
            UPDATE device
            SET address = ?, discovered = ?, mid = ?, vrn = ?, zone = ?, parent = ?, enumerated = ?, productId = ?,
                metadata = ?
            WHERE id = ?;
            )"
        );
        stmt.bind(1, device.getAddress());
        stmt.bind(2, device.isDiscovered());
        stmt.bind(3, device.getMid());
        stmt.bind(4, device.getVrn());
        stmt.bind(5, device.getZone());
        if (device.getParent() == std::nullopt) {
            stmt.bind(6);
        } else {
            stmt.bind(6, device.getParent().value());
        }
        stmt.bind(7, device.isEnumerated());
        stmt.bind(8, device.getProductId());
        if (device.getMetadata() == nullptr) {
            stmt.bind(9);
        } else {
            stmt.bind(9, *device.getMetadata());
        }
        stmt.bind(10, device.getId());
        try {
            stmt.exec();
        } catch (const SQLite::Exception &e) {
            throw std::runtime_error(
                this->formatErrorMessage(
                    "Failed to update device entity ID " + std::to_string(device.getId()),
                    e.what()
                )
            );
        }
    }

    void remove(const uint32_t id) {
        SQLite::Statement stmt(*m_db,
            R"(
            DELETE FROM device
            WHERE id = ?;
            )"
        );
        stmt.bind(1, id);
        stmt.exec();
    }

    std::set<uint8_t> getAddresses() {
        std::set<uint8_t> addrs;
        SQLite::Statement stmt(*m_db, "SELECT d.address FROM device as d");
        while(stmt.executeStep()) {
            addrs.insert(static_cast<uint8_t>(stmt.getColumn(0).getUInt()));
        }
        return addrs;
    }

    std::optional<uint16_t> getHwpidByAddress(const uint8_t addr) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT p.hwpid
            FROM products as p INNER JOIN device as d
            ON p.id = d.productId
            WHERE d.address = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, addr);
        if (!stmt.executeStep()) {
            return std::nullopt;
        }
        return static_cast<uint16_t>(stmt.getColumn(0).getUInt());
    }

    std::optional<uint32_t> getMidByAddress(const uint8_t addr) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT mid
            FROM device
            WHERE address = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, addr);
        if (!stmt.executeStep()) {
            return std::nullopt;
        }
        return stmt.getColumn(0).getUInt();
    }

    std::shared_ptr<std::string> getMetadataByAddress(const uint8_t addr) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT metadata
            FROM device
            WHERE address = ?
            LIMIT 1;
            )"
        );
        stmt.bind(1, addr);
        if (!stmt.executeStep() || stmt.getColumn(0).isNull()) {
            return nullptr;
        } else {
            return std::make_shared<std::string>(stmt.getColumn(0).getString());
        }
    }

    bool implementsPeripheral(const uint32_t id, const int16_t peripheral) {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT COUNT(*)
            FROM device as d
            INNER JOIN productDriver as pd ON pd.productId = d.productId
            INNER JOIN driver as drv ON drv.id = pd.driverId
            WHERE d.id = ? and drv.peripheralNumber = ?
            )"
        );
        stmt.bind(1, id);
        stmt.bind(2, peripheral);
        if (!stmt.executeStep()) {
            return false;
        }
        return stmt.getColumn(0).getUInt() > 0;
    }

    std::map<uint8_t, uint32_t> getAddressProductIdMap() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT address, productId
            FROM device;
            )"
        );
        std::map<uint8_t, uint32_t> map;
        while (stmt.executeStep()) {
            map.insert_or_assign(
                static_cast<uint8_t>(stmt.getColumn(0).getUInt()),
                stmt.getColumn(1).getUInt()
            );
        }
        return map;
    }

    std::set<uint8_t> getProductAddresses(const uint32_t productId) {
        SQLite::Statement stmt(*m_db,
            R"(
                SELECT address
                FROM device
                WHERE productId = ?;
            )"
        );
        stmt.bind(1, productId);
        std::set<uint8_t> set;
        while (stmt.executeStep()) {
            set.insert(static_cast<uint8_t>(stmt.getColumn(0).getUInt()));
        }
        return set;
    }

    std::vector<std::pair<Device, Product>> getDeviceProductPairs(const std::vector<uint8_t>& requestedDevices) {
        std::vector<Device> devVec;
        if (requestedDevices.size() == 0) {
            devVec = this->getAll();
        } else {
            for (const auto addr : requestedDevices) {
                auto dev = this->getByAddress(addr);
                if (dev != nullptr) {
                    devVec.emplace_back(*dev);
                }
            }
        }
        std::vector<std::pair<Device, Product>> vec;
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT id, hwpid, hwpidVersion, osBuild, osVersion, dpaVersion, handlerUrl, handlerHash, customDriver,
                packageId, name
            FROM product
            WHERE id = ?
            LIMIT 1;
            )"
        );
        for (const auto& dev : devVec) {
            stmt.bind(1, dev.getProductId());
            if (stmt.executeStep()) {
                vec.emplace_back(dev, Product::fromResult(stmt));
            }
            stmt.reset();
        }
        return vec;
    }

    std::map<uint8_t, iqrf::embed::node::NodeMidHwpid> getNodeMidHwpidMap() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT d.address, d.mid, p.hwpid
            FROM device as d
            INNER JOIN product as p ON p.id = d.productId
            WHERE address > ?;
            )"
        );
        std::map<uint8_t, iqrf::embed::node::NodeMidHwpid> map;
        while (stmt.executeStep()) {
            map.insert(
                std::make_pair(
                    static_cast<uint8_t>(stmt.getColumn(0).getUInt()),
                    iqrf::embed::node::NodeMidHwpid(
                        stmt.getColumn(1).getUInt(),
                        static_cast<uint16_t>(stmt.getColumn(2).getUInt())
                    )
                )
            );
        }
        return map;
    }
};

}
