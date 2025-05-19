#pragma once

#include <set>

#include <models/migration.hpp>
#include <repositories/base_repo.hpp>

using iqrf::db::models::Migration;

namespace iqrf::db::repos {

class MigrationRepository : public BaseRepository {
public:
    using BaseRepository::BaseRepository;

    std::vector<Migration> getAll() {
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT version, executedAt
            FROM migrations;
            )"
        );
        std::vector<Migration> vec;
        while (stmt.executeStep()) {
            vec.emplace_back(Migration::fromResult(stmt));
        }
        return vec;
    }

    std::set<std::string> getExecutedVersions() {
        std::set<std::string> migrations;
        SQLite::Statement stmt(*m_db,
            R"(
            SELECT version
            FROM migrations;
            )"
        );
        while(stmt.executeStep()) {
            migrations.insert(stmt.getColumn(0).getString());
        }
        return migrations;
    }
};

}
