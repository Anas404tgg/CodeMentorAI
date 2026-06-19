#ifndef MIGRATIONS_H
#define MIGRATIONS_H

#include <sqlite3.h>

/**
 * @brief Runs database migrations (creates tables if they don't exist).
 * @param db The database connection.
 * @return 0 on success, -1 on failure.
 */
int run_migrations(sqlite3 *db);

#endif // MIGRATIONS_H