#include "transactions.h"
#include "db.h"
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h> // for send()
#include <sys/socket.h>
#include <string.h>

#define BUFFER_SIZE 1024


void get_transaction_history(const char *username, int client_socket) {
    sqlite3_stmt *stmt;
    char query[300];

    sprintf(query,
        "SELECT sender, receiver, amount, timestamp FROM transactions "
        "WHERE sender = ? OR receiver = ? ORDER BY timestamp DESC");

    if (sqlite3_prepare_v2(get_db_connection(), query, -1, &stmt, 0) != SQLITE_OK) {
        send(client_socket, "Failed to prepare transaction query.\n", 37, 0);
        return;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);

    char response[BUFFER_SIZE];
    strcpy(response, "Transaction History:\n");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *sender = sqlite3_column_text(stmt, 0);
        const unsigned char *receiver = sqlite3_column_text(stmt, 1);
        double amount = sqlite3_column_double(stmt, 2);
        const unsigned char *timestamp = sqlite3_column_text(stmt, 3);

        char line[200];
        sprintf(line, "%s -> %s : $%.2f at %s\n", sender, receiver, amount, timestamp);
        strcat(response, line);
    }

    sqlite3_finalize(stmt);
    send(client_socket, response, strlen(response), 0);
}