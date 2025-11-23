-- Migration 002: Create comments table

BEGIN TRANSACTION;

-- Create comments table
CREATE TABLE comments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    post_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    body TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (post_id) REFERENCES posts(id),
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- Record migration
INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('002', datetime('now'));

COMMIT;
