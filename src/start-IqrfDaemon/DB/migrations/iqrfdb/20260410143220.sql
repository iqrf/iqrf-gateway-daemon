PRAGMA foreign_keys=off;

BEGIN TRANSACTION;

-- Add manufacturer column to product table

CREATE TABLE IF NOT EXISTS 'product_new' (
  'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  'hwpid' INTEGER NOT NULL,
  'hwpidVersion' INTEGER NOT NULL,
  'osBuild' INTEGER NOT NULL,
  'osVersion' TEXT NOT NULL,
  'dpaVersion' INTEGER NOT NULL,
  'handlerUrl' TEXT DEFAULT NULL,
  'handlerHash' TEXT DEFAULT NULL,
  'customDriver' TEXT DEFAULT NULL,
  'packageId' INTEGER DEFAULT NULL,
  'manufacturer' TEXT DEFAULT NULL,
  'name' TEXT DEFAULT NULL
);

INSERT INTO 'product_new' ('id', 'hwpid', 'hwpidVersion', 'osBuild', 'osVersion', 'dpaVersion', 'handlerUrl', 'handlerHash', 'customDriver', 'packageId', 'name')
  SELECT `id`, `hwpid`, `hwpidVersion`, `osBuild`, `osVersion`, `dpaVersion`, `handlerUrl`, `handlerHash`, `customDriver`, `packageId`, `name` FROM 'product';
DROP TABLE IF EXISTS 'product';
ALTER TABLE 'product_new' RENAME TO 'product';

INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('20260410143220', datetime('now'));

COMMIT;

PRAGMA foreign_keys=on;
