#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sqlite3.h>  // Needed for database functions
#include "db.h"
#include "auth.h"
#include "transactions.h"

#define PORT 8080
#define BUFFER_SIZE 1024

int server_fd; // Global for graceful shutdown

void handle_shutdown(int sig) {
    printf("\n[INFO] Shutting down server gracefully...\n");
    close(server_fd);
    exit(0);
}

void print_supported_commands() {
    printf("\n> Supported Commands:\n");
    printf("  SIGNUP <username> <password>\n");
    printf("  LOGIN <username> <password>\n");
    printf("  BALANCE\n");
    printf("  TRANSFER <recipient> <amount>\n");
    printf("  HISTORY\n");
    printf("  SHOW_ALL_USERS\n");
    printf("  ADMIN_STATS\n\n");
}

// This version sends the transaction history to client socket
void get_transaction_history_socket(const char *username, int sock) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *sql = "SELECT timestamp, sender, receiver, amount FROM transactions WHERE sender=? OR receiver=? ORDER BY timestamp DESC";

    if (sqlite3_open("wallet.db", &db) != SQLITE_OK) {
        send(sock, "Database error.\n", 16, 0);
        return;
    }

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        send(sock, "Failed to fetch history.\n", 25, 0);
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);

    char row[256];
    send(sock, "Transaction History:\n", 22, 0);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *timestamp = (const char *)sqlite3_column_text(stmt, 0);
        const char *sender = (const char *)sqlite3_column_text(stmt, 1);
        const char *receiver = (const char *)sqlite3_column_text(stmt, 2);
        double amount = sqlite3_column_double(stmt, 3);

        snprintf(row, sizeof(row), "%s | From: %s | To: %s | ₹%.2f\n", timestamp, sender, receiver, amount);
        send(sock, row, strlen(row), 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);
    char buffer[BUFFER_SIZE];
    char current_username[100] = "";

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int read_size = recv(sock, buffer, BUFFER_SIZE, 0);
        if (read_size <= 0) {
            printf("[INFO] Client disconnected from socket %d\n", sock);
            break;
        }

        char command[20], arg1[50], arg2[50];
        double amount;

        if (sscanf(buffer, "SIGNUP %s %s", arg1, arg2) == 2) {
            if (signup_user(arg1, arg2)) {
                send(sock, "Signup successful!\n", 19, 0);
            } else {
                send(sock, "Signup failed! Username might be taken.\n", 40, 0);
            }
        }

        else if (sscanf(buffer, "LOGIN %s %s", arg1, arg2) == 2) {
            if (login_user(arg1, arg2)) {
                strcpy(current_username, arg1);
                send(sock, "Login successful\n", 17, 0);
            } else {
                send(sock, "Login failed\n", 13, 0);
            }
        }

        else if (strncmp(buffer, "BALANCE", 7) == 0) {
            if (strlen(current_username) == 0) {
                send(sock, "Please login first.\n", 21, 0);
                continue;
            }

            double balance = get_balance(current_username);
            char response[50];
            sprintf(response, "Balance: ₹%.2f\n", balance);
            send(sock, response, strlen(response), 0);
        }

        else if (sscanf(buffer, "TRANSFER %s %lf", arg1, &amount) == 2) {
            if (strlen(current_username) == 0) {
                send(sock, "Please login first.\n", 21, 0);
                continue;
            }

            if (amount > 1000.0) {
                send(sock, "Transaction limit exceeded! Max ₹1000.\n", 40, 0);
                continue;
            }

            if (transfer_money(current_username, arg1, amount)) {
                double new_balance = get_balance(current_username);
                char response[BUFFER_SIZE];
                sprintf(response, "Transfer successful! New balance: ₹%.2f\n", new_balance);
                send(sock, response, strlen(response), 0);

                printf("[INFO] %s sent ₹%.2f to %s. New Balance: ₹%.2f\n", current_username, amount, arg1, new_balance);
            } else {
                send(sock, "Transfer failed! Check balance or recipient.\n", 45, 0);
            }
        }

        else if (strncmp(buffer, "HISTORY", 7) == 0) {
            if (strlen(current_username) == 0) {
                send(sock, "Please login first.\n", 21, 0);
                continue;
            }

            get_transaction_history_socket(current_username, sock);
        }

        else if (strncmp(buffer, "SHOW_ALL_USERS", 15) == 0) {
            if (!is_admin(current_username)) {
                send(sock, "Unauthorized. Admin access only.\n", 34, 0);
                continue;
            }

            char result[65536];  // bump it up for testing!
            show_all_users(result);
            send(sock, result, strlen(result), 0);
        }

        else if (strncmp(buffer, "ADMIN_STATS", 11) == 0) {
            if (!is_admin(current_username)) {
                send(sock, "Unauthorized. Admin access only.\n", 34, 0);
                continue;
            }

            char result[65536];  // bump it up for testing!
            get_admin_stats(result);  // Implemented in db.c
            send(sock, result, strlen(result), 0);
        }

        else {
            send(sock, "Invalid command!\n", 17, 0);
        }
    }

    close(sock);
    return NULL;
}

int main() {
    signal(SIGINT, handle_shutdown); // Graceful Ctrl+C shutdown

    if (!initialize_db()) {
        printf("Database initialization failed!\n");
        return 1;
    }

    struct sockaddr_in server, client;
    socklen_t client_size = sizeof(client);
    int client_sock;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(server_fd, 5);

    printf("Server listening on port %d...\n", PORT);
    print_supported_commands();

    while (1) {
        client_sock = accept(server_fd, (struct sockaddr *)&client, &client_size);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        pthread_t thread;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        if (pthread_create(&thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Thread creation failed");
            continue;
        }

        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}