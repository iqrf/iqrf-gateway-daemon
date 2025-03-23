#pragma once

#include <tom/migration.hpp>

namespace Migrations {

    T_MIGRATION

    struct _2025_03_22_000000_init : Migration {

        void up() const override {
            if (!Schema::hasTable("products")) {
                Schema::create("products", [](Blueprint &table) {
                    table.id();
                    table.string("name");
                    table.unsignedSmallInteger("hwpid");
                    table.unsignedSmallInteger("hwpid_version");
                    table.unsignedSmallInteger("os_build");
                    table.string("os_version", 10);
                    table.unsignedSmallInteger("dpa_version");
                    table.string("handler_url");
                    table.string("handler_hash");
                    table.text("custom_driver");
                    table.unsignedInteger("package_id").nullable();
                    table.unique({"hwpid", "hwpid_version", "os_build", "dpa_version"});
                });
            }
            if (!Schema::hasTable("drivers")) {
                Schema::create("drivers", [](Blueprint &table) {
                    table.id();
                    table.string("name");
                    table.integer("peripheral_number");
                    table.Float("version");
                    table.unsignedInteger("version_flags");
                    table.text("driver");
                    table.string("driver_hash", 64);
                    table.unique({"peripheral_number", "version"});
                });
            }
            if (!Schema::hasTable("product_drivers")) {
                Schema::create("product_drivers", [](Blueprint &table) {
                    table.unsignedBigInteger("product_id");
                    table.unsignedBigInteger("driver_id");
                    table.primary({"product_id", "driver_id"});
                    table.foreign("product_id").references(ID).on("products");
                    table.foreign("driver_id").references(ID).on("drivers");
                });
            }
            if (!Schema::hasTable("devices")) {
                Schema::create("devices", [](Blueprint &table) {
                    table.id();
                    table.unsignedTinyInteger("address").unique();
                    table.boolean("discovered");
                    table.unsignedInteger("mid").unique();
                    table.unsignedTinyInteger("vrn");
                    table.unsignedTinyInteger("zone");
                    table.unsignedTinyInteger("parent").nullable();
                    table.boolean("enumerated");
                    table.json("metadata").nullable();
                    table.unsignedBigInteger("product_id");
                    table.foreign("product_id").references(ID).on("products");
                });
            }
            if (!Schema::hasTable("binary_outputs")) {
                Schema::create("binary_outputs", [](Blueprint &table) {
                    table.id();
                    table.unsignedTinyInteger("count");
                    table.unsignedBigInteger("device_id");
                    table.foreign("device_id").references(ID).on("devices");
                });
            }
            if (!Schema::hasTable("lights")) {
                Schema::create("lights", [](Blueprint &table) {
                    table.id();
                    table.unsignedBigInteger("device_id");
                    table.foreign("device_id").references(ID).on("devices");
                });
            }
            if (!Schema::hasTable("sensors")) {
                Schema::create("sensors", [](Blueprint &table) {
                    table.id();
                    table.unsignedTinyInteger("type");
                    table.string("name");
                    table.string("short_name");
                    table.string("unit");
                    table.unsignedTinyInteger("decimals");
                    table.boolean("frc_2bit");
                    table.boolean("frc_byte");
                    table.boolean("frc_2byte");
                    table.boolean("frc_4byte");
                    table.unique({"type", "name"});
                });
            }
            if (!Schema::hasTable("device_sensors")) {
                Schema::create("device_sensors", [](Blueprint &table) {
                    table.id();
                    table.unsignedTinyInteger("global_index");
                    table.unsignedTinyInteger("type_index");
                    table.datetime("last_updated").nullable();
                    table.Double("last_value").nullable();
                    table.json("metadata").nullable();
                    table.unsignedBigInteger("device_id");
                    table.unsignedBigInteger("sensor_id");
                    table.foreign("device_id").references(ID).on("devices").cascadeOnDelete();
                    table.foreign("sensor_id").references(ID).on("sensors");
                    table.unique({"device_id", "sensor_id", "global_index"});
                });
            }
        }

        void down() const override {
            Schema::dropIfExists("device_sensors");
            Schema::dropIfExists("sensors");
            Schema::dropIfExists("lights");
            Schema::dropIfExists("binary_outputs");
            Schema::dropIfExists("devices");
            Schema::dropIfExists("product_drivers");
            Schema::dropIfExists("drivers");
            Schema::dropIfExists("products");
        }

    };
}
