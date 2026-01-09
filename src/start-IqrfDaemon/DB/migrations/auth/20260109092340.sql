BEGIN TRANSACTION;

-- API token table update revoked to status column

CREATE TABLE 'api_tokens_new' (
  'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  'owner' TEXT NOT NULL,
  'salt' TEXT NOT NULL,
  'hash' TEXT NOT NULL,
  'createdAt' INTEGER NOT NULL CHECK(createdAt > 0),
  'expiresAt' INTEGER NOT NULL CHECK(expiresAt > createdAt),
  'status' INTEGER NOT NULL CHECK(status IN (0, 1, 2)),
  'service' INTEGER NOT NULL
);

-- Migrate old data into new

INSERT INTO 'api_tokens_new' ('id', 'owner', 'salt', 'hash', 'createdAt', 'expiresAt', 'status', 'service')
  SELECT
    id,
    owner,
    salt,
    hash,
    createdAt,
    expiresAt,
    CASE
      WHEN revoked = 1 THEN 2
      WHEN expiresAt < strftime("%s", 'now') THEN 1
      ELSE 0
    END,
    service
  FROM 'api_tokens';

DROP TABLE 'api_tokens';
ALTER TABLE 'api_tokens_new' RENAME TO 'api_tokens';

INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('20260109092340', datetime('now'));

COMMIT;
