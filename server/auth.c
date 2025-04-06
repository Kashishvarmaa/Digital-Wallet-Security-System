#include "auth.h"
#include "db.h"
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

int signup_user(const char *username, const char *password) {
    char query[200];
    sprintf(query, "INSERT INTO users (username, password) VALUES ('%s', '%s');", username, password);
    return execute_query(query);
}

int login_user(const char *username, const char *password) {
    sqlite3_stmt *stmt;
    char query[200];
    sprintf(query, "SELECT * FROM users WHERE username='%s' AND password='%s';", username, password);

    sqlite3_prepare_v2(get_db_connection(), query, -1, &stmt, NULL);
    int result = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return result;
}