BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS `Node` (
	`Id`	INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Nadr`	INTEGER NOT NULL,
	`Hwpid`	INTEGER NOT NULL
);
COMMIT;
