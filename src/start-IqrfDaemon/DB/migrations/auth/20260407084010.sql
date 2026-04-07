BEGIN TRANSACTION;

-- API token table drop salt

CREATE TABLE 'api_tokens_new' (
  'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  'owner' TEXT NOT NULL,
  'hash' TEXT NOT NULL,
  'createdAt' TEXT NOT NULL,
  'expiresAt' TEXT NOT NULL,
  'status' INTEGER NOT NULL CHECK(status IN (0, 1, 2)),
  'service' INTEGER NOT NULL,
  'invalidatedAt' TEXT DEFAULT NULL
);

DROP TABLE 'api_tokens';
ALTER TABLE 'api_tokens_new' RENAME TO 'api_tokens';

INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('20260407084010', datetime('now'));

COMMIT;
