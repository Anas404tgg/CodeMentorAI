/**
 * @file migrations.c
 * @brief Database migration module for CodeMentor AI.
 *
 * Runs the schema to create tables if they don't exist.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "migrations.h"

/**
 * @brief Embedded schema string.
 *        This is the content of docs/schema.sql.
 */
static const char *schema_sql =
    "-- SQLite schema for CodeMentor AI\n"
    "-- Tables: users, submissions, analyses, quizzes\n\n"
    "-- Users table\n"
    "CREATE TABLE IF NOT EXISTS users (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    username TEXT UNIQUE NOT NULL,\n"
    "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"
    ");\n\n"
    "-- Submissions table\n"
    "CREATE TABLE IF NOT EXISTS submissions (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    user_id INTEGER NOT NULL,\n"
    "    code TEXT NOT NULL,\n"
    "    submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
    "    FOREIGN KEY (user_id) REFERENCES users(id)\n"
    ");\n\n"
    "-- Analyses table\n"
    "CREATE TABLE IF NOT EXISTS analyses (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    submission_id INTEGER NOT NULL,\n"
    "    lines INTEGER NOT NULL,\n"
    "    functions INTEGER NOT NULL,\n"
    "    max_nesting_depth INTEGER NOT NULL,\n"
    "    dangerous_patterns INTEGER NOT NULL,\n"
    "    exit_code INTEGER NOT NULL,\n"
    "    timed_out INTEGER NOT NULL,\n"
    "    stdout TEXT,\n"
    "    stderr TEXT,\n"
    "    ai_feedback TEXT,\n"
    "    analyzed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
    "    FOREIGN KEY (submission_id) REFERENCES submissions(id)\n"
    ");\n\n"
    "-- Quizzes table (for future use, optional)\n"
    "CREATE TABLE IF NOT EXISTS quizzes (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    title TEXT NOT NULL,\n"
    "    description TEXT,\n"
    "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP\n"
    ");\n\n"
    "-- Quiz questions table (optional)\n"
    "CREATE TABLE IF NOT EXISTS quiz_questions (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    quiz_id INTEGER NOT NULL,\n"
    "    question_text TEXT NOT NULL,\n"
    "    correct_answer TEXT, -- For simplicity, we store the correct answer as text\n"
    "    FOREIGN KEY (quiz_id) REFERENCES quizzes(id)\n"
    ");\n\n"
    "-- User quiz attempts table (optional)\n"
    "CREATE TABLE IF NOT EXISTS quiz_attempts (\n"
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
    "    user_id INTEGER NOT NULL,\n"
    "    quiz_id INTEGER NOT NULL,\n"
    "    score INTEGER NOT NULL,\n"
    "    attempted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,\n"
    "    FOREIGN KEY (user_id) REFERENCES users(id),\n"
    "    FOREIGN KEY (quiz_id) REFERENCES quizzes(id)\n"
    ");";

/**
 * @brief Runs database migrations (creates tables if they don't exist).
 * @param db The database connection.
 * @return 0 on success, -1 on failure.
 */
int run_migrations(sqlite3 *db) {
    if (db == NULL) {
        fprintf(stderr, "Database connection is NULL\n");
        return -1;
    }

    // Execute the schema string
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, schema_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }

    return 0;
}