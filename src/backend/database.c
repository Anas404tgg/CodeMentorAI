/**
 * @file database.c
 * @brief SQLite3 database operations for CodeMentor AI.
 *
 * Implements CRUD operations for users, submissions, and analyses.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "database.h"
#include "parser.h"     // For Metrics
#include "sandbox.h"    // For SandboxResult
#include "json_utils.h" // For JSON helpers (to be implemented)
#include "migrations.h" // For run_migrations

/* Static database connection */
static sqlite3 *db = NULL;

/**
 * @brief Executes a SQL statement and checks for errors.
 * @param sql The SQL statement to execute.
 * @return 0 on success, -1 on failure.
 */
static int exec_sql(const char *sql) {
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    return 0;
}

/**
 * @brief Initializes the database connection and runs migrations.
 * @return 0 on success, -1 on failure.
 */
int database_init(void) {
    int rc = sqlite3_open("codementor.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // Enable foreign keys
    exec_sql("PRAGMA foreign_keys = ON;");

    // Run migrations
    return run_migrations(db);
}

/**
 * @brief Cleans up the database connection.
 */
void database_cleanup(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

/**
 * @brief Creates a new user.
 * @param username The username for the new user.
 * @param user_id Output parameter for the new user's ID.
 * @return 0 on success, -1 on failure.
 */
int database_create_user(const char *username, int *user_id) {
    if (!username || !user_id) return -1;

    const char *sql = "INSERT INTO users (username) VALUES (?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    *user_id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return 0;
}

/**
 * @brief Retrieves a user by username.
 * @param username The username to search for.
 * @param user_id Output parameter for the user's ID.
 * @return 0 on success, -1 if not found or error.
 */
int database_get_user_by_username(const char *username, int *user_id) {
    if (!username || !user_id) return -1;

    const char *sql = "SELECT id FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *user_id = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return 0;
    } else if (rc == SQLITE_DONE) {
        // No rows
        sqlite3_finalize(stmt);
        return -1;
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }
}

/**
 * @brief Stores a code submission.
 * @param user_id The ID of the user making the submission.
 * @param code The C source code submitted.
 * @param submission_id Output parameter for the new submission's ID.
 * @return 0 on success, -1 on failure.
 */
int database_create_submission(int user_id, const char *code, int *submission_id) {
    if (!code || !submission_id) return -1;

    const char *sql = "INSERT INTO submissions (user_id, code) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, code, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    *submission_id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return 0;
}

/**
 * @brief Stores an analysis result for a submission.
 * @param submission_id The ID of the submission being analyzed.
 * @param metrics The code metrics from the parser.
 * @param sandbox_result The result from the sandbox execution.
 * @param ai_feedback The feedback string from the AI.
 * @param analysis_id Output parameter for the new analysis' ID.
 * @return 0 on success, -1 on failure.
 */
int database_create_analysis(int submission_id,
                             const Metrics *metrics,
                             const SandboxResult *sandbox_result,
                             const char *ai_feedback,
                             int *analysis_id) {
    if (!metrics || !sandbox_result || !analysis_id) return -1;

    const char *sql =
        "INSERT INTO analyses ("
        "submission_id, lines, functions, max_nesting_depth, dangerous_patterns, "
        "exit_code, timed_out, stdout, stderr, ai_feedback"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, submission_id);
    sqlite3_bind_int(stmt, 2, metrics->lines);
    sqlite3_bind_int(stmt, 3, metrics->functions);
    sqlite3_bind_int(stmt, 4, metrics->max_nesting_depth);
    sqlite3_bind_int(stmt, 5, metrics->dangerous_patterns);
    sqlite3_bind_int(stmt, 6, sandbox_result->exit_code);
    sqlite3_bind_int(stmt, 7, sandbox_result->timed_out);
    sqlite3_bind_text(stmt, 8, sandbox_result->stdout_buf, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, sandbox_result->stderr_buf, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 10, ai_feedback ? ai_feedback : "", -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    *analysis_id = sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return 0;
}

/**
 * @brief Retrieves the history of submissions for a user.
 *        Returns a JSON array of objects, each representing an analysis with
 *        submission code and metrics.
 * @param user_id The ID of the user.
 * @param limit Maximum number of rows to return.
 * @param offset Offset for pagination.
 * @param[out] rows Pointer to an array of strings (each string is a JSON object).
 * @param[out] count Number of rows returned.
 * @return 0 on success, -1 on failure.
 */
int database_get_history(int user_id, int limit, int offset, char ***rows, int *count) {
    if (!rows || !count) return -1;

    const char *sql =
        "SELECT "
        "s.id, s.code, s.submitted_at, "
        "a.lines, a.functions, a.max_nesting_depth, a.dangerous_patterns, "
        "a.exit_code, a.timed_out, a.stdout, a.stderr, a.ai_feedback, a.analyzed_at "
        "FROM submissions s "
        "JOIN analyses a ON s.id = a.submission_id "
        "WHERE s.user_id = ? "
        "ORDER BY s.submitted_at DESC "
        "LIMIT ? OFFSET ?;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_int(stmt, 2, limit);
    sqlite3_bind_int(stmt, 3, offset);

    int capacity = 10;
    int size = 0;
    char **result = malloc(capacity * sizeof(char *));
    if (!result) {
        sqlite3_finalize(stmt);
        return -1;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        // We'll build a JSON string for each row.
        // Note: This does not escape special characters in the text fields.
        // In a production environment, we should use a proper JSON library.
        char buffer[4096]; // Increased size for safety
        int len = snprintf(buffer, sizeof(buffer),
            "{"
            "\"submission_id\":%d,"
            "\"code\":\"%s\","
            "\"submitted_at\":\"%s\","
            "\"lines\":%d,"
            "\"functions\":%d,"
            "\"max_nesting_depth\":%d,"
            "\"dangerous_patterns\":%d,"
            "\"exit_code\":%d,"
            "\"timed_out\":%d,"
            "\"stdout\":\"%s\","
            "\"stderr\":\"%s\","
            "\"ai_feedback\":\"%s\","
            "\"analyzed_at\":\"%s\""
            "}",
            sqlite3_column_int(stmt, 0),
            (const char *)sqlite3_column_text(stmt, 1),
            (const char *)sqlite3_column_text(stmt, 2),
            sqlite3_column_int(stmt, 3),
            sqlite3_column_int(stmt, 4),
            sqlite3_column_int(stmt, 5),
            sqlite3_column_int(stmt, 6),
            sqlite3_column_int(stmt, 7),
            sqlite3_column_int(stmt, 8),
            (const char *)sqlite3_column_text(stmt, 9),
            (const char *)sqlite3_column_text(stmt, 10),
            (const char *)sqlite3_column_text(stmt, 11),
            (const char *)sqlite3_column_text(stmt, 12)
        );

        if (len < 0 || len >= (int)sizeof(buffer)) {
            fprintf(stderr, "Buffer too small for history row\n");
            continue;
        }

        if (size >= capacity) {
            capacity *= 2;
            char **tmp = realloc(result, capacity * sizeof(char *));
            if (!tmp) {
                free(result);
                sqlite3_finalize(stmt);
                return -1;
            }
            result = tmp;
        }
        result[size] = strdup(buffer);
        if (!result[size]) {
            // Clean up and return error
            for (int i = 0; i < size; i++) {
                free(result[i]);
            }
            free(result);
            sqlite3_finalize(stmt);
            return -1;
        }
        size++;
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
        // Clean up
        for (int i = 0; i < size; i++) {
            free(result[i]);
        }
        free(result);
        sqlite3_finalize(stmt);
        return -1;
    }

    *rows = result;
    *count = size;
    sqlite3_finalize(stmt);
    return 0;
}

/**
 * @brief Frees the memory allocated by database_get_history.
 * @param rows The array of strings returned by database_get_history.
 * @param count The number of rows.
 */
void database_free_history(char **rows, int count) {
    if (rows) {
        for (int i = 0; i < count; i++) {
            free(rows[i]);
        }
        free(rows);
    }
}