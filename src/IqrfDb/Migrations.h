#pragma once

#include <orm/db.hpp>
#include <tom/application.hpp>


#include "migrations/_2025_03_22_000000_init.h"

using Orm::DatabaseManager;

namespace iqrf::migrations {

    static void migrateDatabase(std::shared_ptr<DatabaseManager> db, bool dry = false) {
        std::vector<char *> argv(2);
        argv[0] = new char[4];
        argv[1] = new char[8];
        std::strcpy(argv[0], "tom");
        std::strcpy(argv[1], "migrate");
        if (dry) {
            argv.resize(3);
            argv[2] = new char[10];
            std::strcpy(argv[2], "--pretend");
        }
        int argc = static_cast<int>(argv.size());
        auto app = Tom::Application(argc, argv.data(), db, "TOM_ENV", "migrations");
        app.migrations<Migrations::_2025_03_22_000000_init>()
            .run();
    }
}
