#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include <stdio.h>

/**
 * @brief Initializes the AI client (e.g., checks for API key).
 * @return 0 on success, -1 on failure.
 */
int ai_client_init(void);

/**
 * @brief Cleans up the AI client.
 */
void ai_client_cleanup(void);

/**
 * @brief Sends a prompt to the AI API and returns the response.
 *        The caller must free the returned string.
 * @param prompt The full prompt to send to the AI.
 * @return Dynamically allocated string containing the AI's response, or NULL on failure.
 */
char *ai_client_send_prompt(const char *prompt);

/**
 * @brief Gets AI feedback for code analysis using Gemini API.
 *        Constructs a Socratic prompt from code, metrics, and sandbox errors.
 *        The caller must free the returned string.
 * @param code The source code that was analyzed
 * @param metrics_json JSON string containing code metrics
 * @param sandbox_errors Errors from sandbox execution (compilation/runtime)
 * @return Dynamically allocated string containing AI feedback, or NULL on failure.
 */
char *ai_get_feedback(const char *code, const char *metrics_json, const char *sandbox_errors);

#endif // AI_CLIENT_H