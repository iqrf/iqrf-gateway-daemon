BEGIN TRANSACTION;

-- Migration table

CREATE TABLE IF NOT EXISTS 'migrations' (
    'version' TEXT PRIMARY KEY NOT NULL,
    'executedAt' TEXT NOT NULL
);

-- Product table

CREATE TABLE IF NOT EXISTS 'product' (
    'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    'hwpid' INTEGER NOT NULL,
    'hwpidVersion' INTEGER NOT NULL,
    'osBuild' INTEGER NOT NULL,
    'osVersion' TEXT NOT NULL,
    'dpaVersion' INTEGER NOT NULL,
    'handlerUrl' TEXT,
    'handlerHash' TEXT,
    'customDriver' TEXT,
    'packageId' INTEGER NOT NULL,
    'standardEnumerated' INTEGER NOT NULL
);

-- Driver table

CREATE TABLE IF NOT EXISTS 'driver' (
    'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    'name' TEXT NOT NULL,
    'peripheralNumber' INTEGER NOT NULL,
    'version' REAL NOT NULL,
    'versionFlags' INTEGER NOT NULL,
    'driver' TEXT NOT NULL,
    'driverHash' TEXT NOT NULL,
    UNIQUE('peripheralNumber', 'version')
);

-- Product driver table

CREATE TABLE IF NOT EXISTS "productDriver" (
    "productId" INTEGER NOT NULL,
    "driverId" INTEGER NOT NULL,
    FOREIGN KEY("driverId") REFERENCES "driver"("id"),
    FOREIGN KEY("productId") REFERENCES "product"("id"),
    PRIMARY KEY("productId", "driverId")
);

-- Device table

CREATE TABLE IF NOT EXISTS "device" (
    "id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    "address" INTEGER UNIQUE NOT NULL,
    "discovered" INTEGER NOT NULL,
    "mid" INTEGER UNIQUE NOT NULL,
    "vrn" INTEGER NOT NULL,
    "zone" INTEGER NOT NULL,
    "parent" INTEGER,
    "enumerated" INTEGER NOT NULL,
    "productId" INTEGER NOT NULL,
    "name" TEXT,
    "location" TEXT,
    "metadata" TEXT,
    FOREIGN KEY("productId") REFERENCES "product"("id")
);

-- BinaryOutput standard table

CREATE TABLE IF NOT EXISTS "bo" (
    "id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    "deviceId" INTEGER NOT NULL,
    "count" INTEGER NOT NULL,
    FOREIGN KEY("deviceId") REFERENCES "device"("id") ON DELETE CASCADE
);

-- LIGHT standard table

CREATE TABLE IF NOT EXISTS "light" (
    "id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    "deviceId" INTEGER NOT NULL,
    FOREIGN KEY("deviceId") REFERENCES "device"("id") ON DELETE CASCADE
);

-- Sensor standard table

CREATE TABLE IF NOT EXISTS "sensor" (
    "id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    "type" INTEGER NOT NULL,
    "name" TEXT NOT NULL,
    "shortname" TEXT NOT NULL,
    "unit" TEXT NOT NULL,
    "decimals" INTEGER NOT NULL,
    "frc2Bit" INTEGER NOT NULL,
    "frc1Byte" INTEGER NOT NULL,
    "frc2Byte" INTEGER NOT NULL,
    "frc4Byte" INTEGER NOT NULL,
    UNIQUE("type", "name")
);

-- Device sensor table

CREATE TABLE IF NOT EXISTS "deviceSensor" (
    "address" INTEGER NOT NULL,
    "type" INTEGER NOT NULL,
    "globalIndex" INTEGER NOT NULL,
    "typeIndex" INTEGER NOT NULL,
    "sensorId" INTEGER NOT NULL,
    "value" REAL,
    "updated" TEXT,
    "metadata" TEXT,
    FOREIGN KEY("address") REFERENCES "device"("address") ON DELETE CASCADE,
    FOREIGN KEY("sensorId") REFERENCES "sensor"("id"),
    PRIMARY KEY("address", "type", "globalIndex")
);

INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('20240612125412', datetime('now'));

COMMIT;
