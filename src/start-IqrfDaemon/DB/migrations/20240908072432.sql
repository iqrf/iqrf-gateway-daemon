PRAGMA foreign_keys=off;

BEGIN TRANSACTION;

-- Product table update

CREATE TABLE IF NOT EXISTS 'product_new' (
    'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    'hwpid' INTEGER NOT NULL,
    'hwpidVersion' INTEGER NOT NULL,
    'osBuild' INTEGER NOT NULL,
    'osVersion' TEXT NOT NULL,
    'dpaVersion' INTEGER NOT NULL,
    'handlerUrl' TEXT,
    'handlerHash' TEXT,
    'customDriver' TEXT,
    'packageId' INTEGER,
    'name' TEXT
);

INSERT INTO 'product_new' ('id', 'hwpid', 'hwpidVersion', 'osBuild', 'osVersion', 'dpaVersion', 'handlerUrl', 'handlerHash', 'customDriver', 'packageId')
    SELECT `id`, `hwpid`, `hwpidVersion`, `osBuild`, `osVersion`, `dpaVersion`, `handlerUrl`, `handlerHash`, `customDriver`, `packageId` FROM 'product';
DROP TABLE IF EXISTS 'product';
ALTER TABLE 'product_new' RENAME TO 'product';

INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('20240908072432', datetime('now'));

COMMIT;

PRAGMA foreign_keys=on;
