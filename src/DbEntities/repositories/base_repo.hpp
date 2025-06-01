#pragma once

#include <memory>

#include <SQLiteCpp/SQLiteCpp.h>

namespace iqrf::db::repos {

class BaseRepository {
public:
    BaseRepository(std::shared_ptr<SQLite::Database> db) : m_db(db) {}

    ~BaseRepository() = default;
protected:

    std::string formatErrorMessage(std::string description, std::string exmsg) {
        std::ostringstream oss;
        oss << description << ": " << exmsg;
        return oss.str();
    }

    std::shared_ptr<SQLite::Database> m_db;
};

}
