-- Migration 002: Initial migration, users and posts

BEGIN TRANSACTION;

-- Create migrations table

CREATE TABLE IF NOT EXISTS 'migrations' (
    'version' TEXT PRIMARY KEY NOT NULL,
    'executedAt' TEXT NOT NULL
);

-- Create users table
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    email TEXT NOT NULL
);

-- Create posts table
CREATE TABLE posts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    title TEXT NOT NULL,
    content TEXT,
    published_at DATETIME,
    FOREIGN KEY (user_id) REFERENCES users(id)
);

-- Record migration
INSERT INTO 'migrations' ('version', 'executedAt') VALUES ('001', datetime('now'));

COMMIT;
