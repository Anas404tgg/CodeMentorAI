#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <stdbool.h>
#include "parser.h"     // For Metrics
#include "sandbox.h"    // For SandboxResult

/**
 * @brief Initializes the database connection and runs migrations.
 * @return 0 on success, -1 on failure.
 */
int database_init(void);

/**
 * @brief Cleans up the database connection.
 */
void database_cleanup(void);

/**
 * @brief Creates a new user.
 * @param username The username for the new user.
 * @param user_id Output parameter for the new user's ID.
 * @return 0 on success, -1 on failure.
 */
int database_create_user(const char *username, int *user_id);

/**
 * @brief Retrieves a user by username.
 * @param username The username to search for.
 * @param user_id Output parameter for the user's ID.
 * @return 0 on success, -1 if not found or error.
 */
int database_get_user_by_username(const char *username, int *user_id);

/**
 * @brief Stores a code submission.
 * @param user_id The ID of the user making the submission.
 * @param code The C source code submitted.
 * @param submission_id Output parameter for the new submission's ID.
 * @return 0 on success, -1 on failure.
 */
int database_create_submission(int user_id, const char *code, int *submission_id);

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
                             int *analysis_id);

/**
 * @brief Retrieves the history of submissions for a user.
 *        This is a simplified version; in practice, we might want to paginate.
 * @param user_id The ID of the user.
 * @param limit Maximum number of rows to return.
 * @param offset Offset for pagination.
 * @note For simplicity, this function returns a dynamically allocated array of rows.
 *       The caller must free the returned array and each row's strings.
 * @param[out] rows Pointer to an array of strings (each string is a JSON object).
 * @param[out] count Number of rows returned.
 * @return 0 on success, -1 on failure.
 */
int database_get_history(int user_id, int limit, int offset, char ***rows, int *count);

/**
 * @brief Frees the memory allocated by database_get_history.
 * @param rows The array of strings returned by database_get_history.
 * @param count The number of rows.
 */
void database_free_history(char **rows, int count);

#endif // DATABASE_H