PRAGMA foreign_keys=off;

BEGIN TRANSACTION;

CREATE TABLE IF NOT EXISTS `Info` (
	`VersionMajor`	INTEGER NOT NULL,
	`VersionMinor`	INTEGER NOT NULL,
	`Hash` INTEGER
);
DELETE FROM `Info`;
INSERT INTO `Info` (`VersionMajor`, `VersionMinor`, `Hash`) VALUES (1, 0, 0);

COMMIT;

PRAGMA foreign_keys=on;
