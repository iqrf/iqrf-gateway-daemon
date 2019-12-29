PRAGMA foreign_keys=off;

BEGIN TRANSACTION;

ALTER TABLE `Node` RENAME TO `Node_old`;

CREATE TABLE IF NOT EXISTS `Node` (
    `Mid` 	INTEGER NOT NULL PRIMARY KEY UNIQUE,
	`DeviceId` INTEGER NOT NULL,
	`MetaData` TEXT
);

INSERT INTO `Node` (`Mid`, `DeviceId`, `MetaData`)
  SELECT `Mid`, `DeviceId`, `MetaData`
  FROM `Node_old`;

COMMIT;

PRAGMA foreign_keys=on;
