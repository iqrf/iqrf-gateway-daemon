BEGIN TRANSACTION;

CREATE TABLE IF NOT EXISTS `Bonded` (
	`Nadr` INTEGER NOT NULL PRIMARY KEY UNIQUE,
	`Dis` INTEGER NOT NULL,
	`Mid` INTEGER UNIQUE,
	`Enm` INTEGER NOT NULL,
	FOREIGN KEY(`Mid`) REFERENCES `Node`(`Mid`)
);

CREATE TABLE IF NOT EXISTS `Device` (
	`Id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,
	`Hwpid`	INTEGER NOT NULL,
    `HwpidVer`	INTEGER NOT NULL,
    `OsBuild`	INTEGER NOT NULL,
    `DpaVer` INTEGER NOT NULL,
	`RepoPackageId` INTEGER,
	`Notes`	TEXT,
	`HandlerHash`	TEXT,
	`HandlerUrl`	TEXT,
	`CustomDriver`	TEXT,
	`StdEnum` INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS `Driver` (
	`Id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE ,
	`Notes`	TEXT,
	`Name`	TEXT,
	`Version` INTEGER,
	`StandardId` INTEGER,
	`VersionFlag` INTEGER,
	`Driver`	TEXT
);

CREATE TABLE IF NOT EXISTS `DeviceDriver` (
	`DeviceId` INTEGER NOT NULL,
	`DriverId` INTEGER NOT NULL,
	FOREIGN KEY(`DeviceId`) REFERENCES `Device`(`Id`),
	FOREIGN KEY(`DriverId`) REFERENCES `Driver`(`Id`)
);

CREATE TABLE IF NOT EXISTS `Node` (
    `Mid` 	INTEGER NOT NULL PRIMARY KEY UNIQUE,
	`DeviceId` INTEGER,
	`MetaData` TEXT
);

CREATE TABLE IF NOT EXISTS `Sensor` (
	`DeviceId`	INTEGER NOT NULL,
	`Idx`	INTEGER NOT NULL,
    `Sid`	TEXT NOT NULL,
    `Stype`	INTEGER NOT NULL,
    `Name`	TEXT NOT NULL,
    `SName`	TEXT NOT NULL,
    `Unit`	TEXT NOT NULL,
    `Dplac`	INTEGER NOT NULL,
	`Frc2bit`	INTEGER NOT NULL,
	`Frc1byte`	INTEGER NOT NULL,
	`Frc2byte`	INTEGER NOT NULL,
	`Frc4byte`	INTEGER NOT NULL,
	FOREIGN KEY(`DeviceId`) REFERENCES `Device`(`Id`)
);

CREATE TABLE IF NOT EXISTS `Binout` (
	`DeviceId`	INTEGER NOT NULL,
	`Num`	INTEGER NOT NULL,
	FOREIGN KEY(`DeviceId`) REFERENCES `Device`(`Id`)
);

CREATE TABLE IF NOT EXISTS `Dali` (
	`DeviceId`	INTEGER NOT NULL,
	FOREIGN KEY(`DeviceId`) REFERENCES `Device`(`Id`)
);

CREATE TABLE IF NOT EXISTS `Light` (
	`DeviceId`	INTEGER NOT NULL,
	`Num`	INTEGER NOT NULL,
	FOREIGN KEY(`DeviceId`) REFERENCES `Device`(`Id`)
);

COMMIT;
