PRAGMA foreign_keys=off;

BEGIN TRANSACTION;

ALTER TABLE `Driver` RENAME TO `Driver_old`;

CREATE TABLE IF NOT EXISTS `Driver` (
	`Id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE ,
	`Notes`	TEXT,
	`Name`	TEXT,
	`Version` REAL,
	`StandardId` INTEGER,
	`VersionFlag` INTEGER,
	`Driver`	TEXT
);

INSERT INTO `Driver` (`Id`, `Notes`, `Name`, `Version`, `StandardId`, `VersionFlag`, `Driver`)
  SELECT `Id`, `Notes`, `Name`, `Version`, `StandardId`, `VersionFlag`, `Driver`
  FROM `Driver_old`;

CREATE TABLE IF NOT EXISTS `Info` (
	`VersionMajor`	INTEGER NOT NULL,
	`VersionMinor`	INTEGER NOT NULL,
	`Hash` INTEGER
);
DELETE FROM `Info`;
INSERT INTO `Info` (`VersionMajor`, `VersionMinor`, `Hash`) VALUES (1, 0, 0);

COMMIT;

PRAGMA foreign_keys=on;
