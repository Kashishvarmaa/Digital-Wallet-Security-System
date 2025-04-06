-- Reset existing tables
DROP TABLE IF EXISTS transactions;
DROP TABLE IF EXISTS users;

-- Create users table with is_admin flag
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    salt TEXT NOT NULL,
    balance REAL DEFAULT 1000.0,
    is_admin INTEGER DEFAULT 0  -- 0 = not admin, 1 = admin
);

-- Create transactions table
CREATE TABLE transactions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sender TEXT NOT NULL,
    receiver TEXT NOT NULL,
    amount REAL NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (sender) REFERENCES users(username) ON DELETE CASCADE,
    FOREIGN KEY (receiver) REFERENCES users(username) ON DELETE CASCADE
);

-- Insert dummy users (with admin for 'kashish')
INSERT INTO users (username, password, salt, balance, is_admin)
VALUES ('kashish', 'HASHED_PASSWORD_1', 'SALT_1', 5000.0, 1);

INSERT INTO users (username, password, salt, balance, is_admin)
VALUES ('guddu', 'HASHED_PASSWORD_2', 'SALT_2', 3000.0, 0);