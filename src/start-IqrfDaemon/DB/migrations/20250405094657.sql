PRAGMA foreign_keys=off;

BEGIN TRANSACTION;

-- BinaryOutput standard table unique device ID

CREATE TABLE IF NOT EXISTS 'bo_new' (
    'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    'deviceId' INTEGER UNIQUE NOT NULL,
    'count' INTEGER NOT NULL,
    FOREIGN KEY('deviceId') REFERENCES 'device'('id') ON DELETE CASCADE
);

INSERT INTO 'bo_new' ('id', 'deviceId', 'count')
    SELECT MIN(`id`), `deviceId`, `count`
    FROM 'bo'
    GROUP BY 'deviceId';
DROP TABLE IF EXISTS 'bo';
ALTER TABLE 'bo_new' RENAME to 'bo';

-- Light standard table unique device ID

CREATE TABLE IF NOT EXISTS 'light_new' (
    'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    'deviceId' INTEGER UNIQUE NOT NULL,
    FOREIGN KEY('deviceId') REFERENCES 'device'('id') ON DELETE CASCADE
);

INSERT INTO 'light_new' ('id', 'deviceId')
    SELECT MIN(`id`), `deviceId`
    FROM 'light'
    GROUP BY 'deviceId';
DROP TABLE IF EXISTS 'light';
ALTER TABLE `light_new` RENAME to `light`;

INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('20250405094657', datetime('now'));

COMMIT;

PRAGMA foreign_keys=on;
