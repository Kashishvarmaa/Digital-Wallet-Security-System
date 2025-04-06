#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define SALT_SIZE 16
#define HASH_SIZE 64
#define ITERATIONS 100000

sqlite3 *db;

// Initialize the database and create tables if not exist
int initialize_db() {
    if (sqlite3_open("wallet.db", &db) != SQLITE_OK) {
        printf("Failed to open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    char *err_msg = NULL;
    const char *sql_users =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE,"
        "password TEXT,"
        "salt TEXT,"
        "balance REAL DEFAULT 1000.0,"
        "is_admin INTEGER DEFAULT 0);";

    const char *sql_transactions =
        "CREATE TABLE IF NOT EXISTS transactions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT NOT NULL,"
        "receiver TEXT NOT NULL,"
        "amount REAL NOT NULL,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

    if (sqlite3_exec(db, sql_users, NULL, NULL, &err_msg) != SQLITE_OK ||
        sqlite3_exec(db, sql_transactions, NULL, NULL, &err_msg) != SQLITE_OK) {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 0;
    }

    return 1;
}

void close_db() {
    sqlite3_close(db);
}

// Hashing
void hash_password(const char *password, char *salt, char *hashed_password) {
    if (RAND_bytes((unsigned char *)salt, SALT_SIZE) != 1) {
        printf("Random salt generation failed!\n");
        exit(1);
    }

    PKCS5_PBKDF2_HMAC(password, strlen(password), (unsigned char *)salt, SALT_SIZE,
                      ITERATIONS, EVP_sha256(), HASH_SIZE, (unsigned char *)hashed_password);
}

int verify_password(const char *password, const char *stored_salt, const char *stored_hash) {
    char computed_hash[HASH_SIZE];

    PKCS5_PBKDF2_HMAC(password, strlen(password), (unsigned char *)stored_salt, SALT_SIZE,
                      ITERATIONS, EVP_sha256(), HASH_SIZE, (unsigned char *)computed_hash);

    return memcmp(computed_hash, stored_hash, HASH_SIZE) == 0;
}

// User registration
int signup_user(const char *username, const char *password) {
    char salt[SALT_SIZE];
    char hashed_password[HASH_SIZE];

    hash_password(password, salt, hashed_password);

    // Convert binary salt and hash to hex
    char salt_hex[SALT_SIZE * 2 + 1];
    char hash_hex[HASH_SIZE * 2 + 1];
    for (int i = 0; i < SALT_SIZE; i++) snprintf(&salt_hex[i * 2], 3, "%02x", (unsigned char)salt[i]);
    for (int i = 0; i < HASH_SIZE; i++) snprintf(&hash_hex[i * 2], 3, "%02x", (unsigned char)hashed_password[i]);

    const char *sql = "INSERT INTO users (username, password, salt, balance) VALUES (?, ?, ?, 1000.0)";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[ERROR] SQLite prepare failed: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hash_hex, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, salt_hex, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        printf("[ERROR] Signup failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }

    sqlite3_finalize(stmt);
    return 1;
}

// Login
int login_user(const char *username, const char *password) {
    const char *sql = "SELECT password, salt FROM users WHERE username=?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    int result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *stored_hash_hex = (const char *)sqlite3_column_text(stmt, 0);
        const char *stored_salt_hex = (const char *)sqlite3_column_text(stmt, 1);

        char stored_hash[HASH_SIZE];
        char stored_salt[SALT_SIZE];

        for (int i = 0; i < HASH_SIZE; i++) sscanf(&stored_hash_hex[i * 2], "%2hhx", &stored_hash[i]);
        for (int i = 0; i < SALT_SIZE; i++) sscanf(&stored_salt_hex[i * 2], "%2hhx", &stored_salt[i]);

        result = verify_password(password, stored_salt, stored_hash);
    }

    sqlite3_finalize(stmt);
    return result;
}

// Get user balance
double get_balance(const char *username) {
    sqlite3_stmt *stmt;
    double balance = -1;

    const char *sql = "SELECT balance FROM users WHERE username=?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            balance = sqlite3_column_double(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    return balance;
}

// Money transfer
int transfer_money(const char *sender, const char *receiver, double amount) {
    double sender_balance = get_balance(sender);
    if (sender_balance < amount) return 0;

    char *errmsg = NULL;
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    sqlite3_stmt *stmt;
    int success = 1;

    const char *sql1 = "UPDATE users SET balance = balance - ? WHERE username = ?";
    if (sqlite3_prepare_v2(db, sql1, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_double(stmt, 1, amount);
        sqlite3_bind_text(stmt, 2, sender, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) success = 0;
        sqlite3_finalize(stmt);
    } else success = 0;

    const char *sql2 = "UPDATE users SET balance = balance + ? WHERE username = ?";
    if (success && sqlite3_prepare_v2(db, sql2, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_double(stmt, 1, amount);
        sqlite3_bind_text(stmt, 2, receiver, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) success = 0;
        sqlite3_finalize(stmt);
    } else success = 0;

    const char *sql3 = "INSERT INTO transactions (sender, receiver, amount) VALUES (?, ?, ?)";
    if (success && sqlite3_prepare_v2(db, sql3, -1, &stmt, 0) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, sender, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, receiver, -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 3, amount);
        if (sqlite3_step(stmt) != SQLITE_DONE) success = 0;
        sqlite3_finalize(stmt);
    } else success = 0;

    if (success)
        sqlite3_exec(db, "COMMIT;", NULL, NULL, &errmsg);
    else
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, &errmsg);

    if (errmsg) {
        printf("Transaction error: %s\n", errmsg);
        sqlite3_free(errmsg);
    }

    return success;
}

// Execute arbitrary SQL
int execute_query(const char *query) {
    char *errMsg = 0;
    int rc = sqlite3_exec(db, query, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return 0;
    }
    return 1;
}

// For Flask use
sqlite3 *get_db_connection() {
    return db;
}

// Show all users and their transaction history
void show_all_users(char *result) {
    sqlite3 *local_db;
    sqlite3_stmt *stmt;
    const char *sql_users = "SELECT username, password, balance FROM users;";
    const char *sql_history = "SELECT sender, receiver, amount, timestamp FROM transactions WHERE sender = ? OR receiver = ? ORDER BY timestamp DESC;";

    char temp[1024];
    result[0] = '\0';

    if (sqlite3_open("wallet.db", &local_db) != SQLITE_OK) {
        strcpy(result, "Error opening database.\n");
        return;
    }

    if (sqlite3_prepare_v2(local_db, sql_users, -1, &stmt, NULL) != SQLITE_OK) {
        strcpy(result, "Error preparing user query.\n");
        sqlite3_close(local_db);
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *username = (const char *)sqlite3_column_text(stmt, 0);
        const char *password = (const char *)sqlite3_column_text(stmt, 1);
        double balance = sqlite3_column_double(stmt, 2);

        sprintf(temp, "\nUser: %s\nPassword: %s\nBalance: ₹%.2f\nTransaction History:\n", username, password, balance);
        strcat(result, temp);

        sqlite3_stmt *hist_stmt;
        if (sqlite3_prepare_v2(local_db, sql_history, -1, &hist_stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(hist_stmt, 1, username, -1, SQLITE_STATIC);
            sqlite3_bind_text(hist_stmt, 2, username, -1, SQLITE_STATIC);

            while (sqlite3_step(hist_stmt) == SQLITE_ROW) {
                const char *sender = (const char *)sqlite3_column_text(hist_stmt, 0);
                const char *receiver = (const char *)sqlite3_column_text(hist_stmt, 1);
                double amount = sqlite3_column_double(hist_stmt, 2);
                const char *timestamp = (const char *)sqlite3_column_text(hist_stmt, 3);

                sprintf(temp, "  From: %s | To: %s | ₹%.2f | %s\n", sender, receiver, amount, timestamp);
                strcat(result, temp);
            }

            sqlite3_finalize(hist_stmt);
        } else {
            strcat(result, "  Error fetching transaction history.\n");
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(local_db);
}

// Check if user is admin
int is_admin(const char *username) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT is_admin FROM users WHERE username = ?";
    int admin = 1;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            admin = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    return admin;
}



void get_admin_stats(char *response) {
    sqlite3_stmt *stmt;
    char temp[512];
    response[0] = '\0';

    // Total users
    const char *sql_users = "SELECT COUNT(*) FROM users;";
    if (sqlite3_prepare_v2(db, sql_users, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int user_count = sqlite3_column_int(stmt, 0);
            sprintf(temp, "Total Users: %d\n", user_count);
            strcat(response, temp);
        }
        sqlite3_finalize(stmt);
    }

    // Total balance
    const char *sql_balance = "SELECT SUM(balance) FROM users;";
    if (sqlite3_prepare_v2(db, sql_balance, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            double total_balance = sqlite3_column_double(stmt, 0);
            sprintf(temp, "Total Balance in System: ₹%.2f\n", total_balance);
            strcat(response, temp);
        }
        sqlite3_finalize(stmt);
    }

    // Total transactions
    const char *sql_txn = "SELECT COUNT(*) FROM transactions;";
    if (sqlite3_prepare_v2(db, sql_txn, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int txn_count = sqlite3_column_int(stmt, 0);
            sprintf(temp, "Total Transactions: %d\n", txn_count);
            strcat(response, temp);
        }
        sqlite3_finalize(stmt);
    }

    // Top 3 senders
    const char *sql_top_senders =
        "SELECT sender, COUNT(*) as txn_count FROM transactions "
        "GROUP BY sender ORDER BY txn_count DESC LIMIT 3;";
    strcat(response, "\nTop 3 Most Active Senders:\n");

    if (sqlite3_prepare_v2(db, sql_top_senders, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *sender = (const char *)sqlite3_column_text(stmt, 0);
            int count = sqlite3_column_int(stmt, 1);
            sprintf(temp, "  %s - %d transactions\n", sender, count);
            strcat(response, temp);
        }
        sqlite3_finalize(stmt);
    }
}
