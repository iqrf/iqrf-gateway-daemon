BEGIN TRANSACTION;

-- API token table update ISO8601 timestamps and timestamp of invalidation

CREATE TABLE 'api_tokens_new' (
  'id' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
  'owner' TEXT NOT NULL,
  'salt' TEXT NOT NULL,
  'hash' TEXT NOT NULL,
  'createdAt' TEXT NOT NULL,
  'expiresAt' TEXT NOT NULL,
  'status' INTEGER NOT NULL CHECK(status IN (0, 1, 2)),
  'service' INTEGER NOT NULL,
  'invalidatedAt' TEXT DEFAULT NULL
);

-- Migrate old data into new tokens

INSERT INTO 'api_tokens_new' ('id', 'owner', 'salt', 'hash', 'createdAt', 'expiresAt', 'status', 'service', 'invalidatedAt')
  SELECT
    id,
    owner,
    salt,
    hash,
    strftime('%Y-%m-%dT%H:%M:%SZ', createdAt, 'unixepoch'),
    strftime('%Y-%m-%dT%H:%M:%SZ', expiresAt, 'unixepoch'),
    status,
    service,
    CASE
      WHEN status = 1 THEN strftime('%Y-%m-%dT%H:%M:%SZ', expiresAt, 'unixepoch')
      WHEN status = 2 THEN strftime('%Y-%m-%dT%H:%M:%SZ', 'now')
      ELSE NULL
    END
  FROM 'api_tokens';

DROP TABLE 'api_tokens';
ALTER TABLE 'api_tokens_new' RENAME TO 'api_tokens';

INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('20260304175010', datetime('now'));

COMMIT;
