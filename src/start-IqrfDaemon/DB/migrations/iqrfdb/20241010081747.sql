PRAGMA foreign_keys=off;

BEGIN TRANSACTION;

-- Device table update

CREATE TABLE IF NOT EXISTS 'device_new' (
    'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    'address' INTEGER UNIQUE NOT NULL,
    'discovered' INTEGER NOT NULL,
    'mid' INTEGER UNIQUE NOT NULL,
    'vrn' INTEGER NOT NULL,
    'zone' INTEGER NOT NULL,
    'parent' INTEGER,
    'enumerated' INTEGER NOT NULL,
    'productId' INTEGER NOT NULL,
    'metadata' TEXT,
    FOREIGN KEY('productId') REFERENCES 'product'('id')
);

INSERT INTO 'device_new' ('id', 'address', 'discovered', 'mid', 'vrn', 'zone', 'parent', 'enumerated', 'productId', 'metadata')
    SELECT `id`, `address`, `discovered`, `mid`, `vrn`, `zone`, `parent`, `enumerated`, `productId`, `metadata` FROM 'device';
DROP TABLE IF EXISTS 'device';
ALTER TABLE 'device_new' RENAME to 'device';

INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('20241010081747', datetime('now'));

COMMIT;

PRAGMA foreign_keys=on;
