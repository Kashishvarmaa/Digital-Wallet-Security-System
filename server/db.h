#ifndef DB_H
#define DB_H

#include <sqlite3.h>

// Global database variable
extern sqlite3 *db;

// Function declarations
int initialize_db();
int signup_user(const char *username, const char *password);
int login_user(const char *username, const char *password);
double get_balance(const char *username);
int transfer_money(const char *sender, const char *receiver, double amount);
int execute_query(const char *query);
sqlite3 *get_db_connection();
void show_all_users(char *result);
int is_admin(const char *username);
void get_admin_stats(char *response);

#endif