PRAGMA foreign_keys=off;

BEGIN TRANSACTION;

CREATE TABLE IF NOT EXISTS 'api_tokens' (
  'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  'owner' TEXT NOT NULL,
  'salt' TEXT NOT NULL,
  'hash' TEXT NOT NULL,
  'createdAt' INTEGER NOT NULL,
  'expiresAt' INTEGER NOT NULL,
  'revoked' INTEGER NOT NULL,
  'service' INTEGER NOT NULL
);

INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('20251031102110', datetime('now'));

COMMIT;
