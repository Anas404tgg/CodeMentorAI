/**
 * @file server.c
 * @brief HTTP server implementation using libmicrohttpd.
 *
 * Handles API routes for CodeMentor AI: /api/analyze, /api/history, /api/user/:id.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>
#include "server.h"
#include "json_utils.h"   // To be implemented in module 7
#include "parser.h"       // To be implemented in module 3
#include "sandbox.h"      // To be implemented in module 4
#include "database.h"     // To be implemented in module 5
#include "ai_client.h"    // To be implemented in module 6
#include <limits.h>

#define JSON_CONTENT_TYPE "application/json"

/**
 * @brief Connection context for storing POST data
 */
struct ConnectionContext {
    char *post_data;
    size_t post_data_size;
};

/**
 * @brief Creates a JSON response from a string literal.
 * @param json_string The JSON string to send.
 * @param connection Unused connection handle (required by MHD signature).
 * @return MHD response object.
 */
static struct MHD_Response *create_json_response(const char *json_string,
                                                 struct MHD_Connection *connection) {
    (void)connection; // Unused parameter
    struct MHD_Response *response =
        MHD_create_response_from_buffer(strlen(json_string),
                                        (void *)json_string,
                                        MHD_RESPMEM_MUST_COPY);
    if (response == NULL) {
        return NULL;
    }
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE,
                            JSON_CONTENT_TYPE);
    return response;
}

/**
 * @brief Handles the /api/analyze route (POST).
 * @param connection The connection handle.
 * @param url The requested URL.
 * @param method The HTTP method.
 * @return MHD response object.
 */
static struct MHD_Response *handle_analyze(struct MHD_Connection *connection,
                                           const char *url,
                                           const char *method,
                                           const char *post_data) {
    (void)url; // Unused parameter
    if (strcmp(method, "POST") != 0) {
        const char *error = "{\"error\": \"Method not allowed\"}";
        return create_json_response(error, connection);
    }

    if (post_data == NULL) {
        const char *error = "{\"error\": \"No POST data provided\"}";
        return create_json_response(error, connection);
    }

    // Parse JSON to extract "code" and "language" fields
    cJSON *json = cJSON_Parse(post_data);
    if (json == NULL) {
        const char *error = "{\"error\": \"Invalid JSON\"}";
        return create_json_response(error, connection);
    }

    // Extract code and language
    char *code = NULL;

    cJSON *code_item = cJSON_GetObjectItemCaseSensitive(json, "code");
    if (code_item && cJSON_IsString(code_item)) {
        code = code_item->valuestring;
    }

    // Extract language (currently unused but kept for API consistency)
    cJSON *language_item = cJSON_GetObjectItemCaseSensitive(json, "language");
    (void)language_item; // Unused but kept for API consistency

    // Validate required fields
    if (code == NULL) {
        cJSON_Delete(json);
        const char *error = "{\"error\": \"Missing 'code' field\"}";
        return create_json_response(error, connection);
    }

    // For now, we only support C language
    // In a full implementation, we would validate the language field

    // Step 2: Call parser_analyze(code) from parser.c → get local metrics
    Metrics metrics;
    if (parse_c_code(code, &metrics) != 0) {
        cJSON_Delete(json);
        const char *error = "{\"error\": \"Failed to parse code\"}";
        return create_json_response(error, connection);
    }

    // Step 3: Call sandbox_run(code) from sandbox.c → compile with gcc, get output/errors
    SandboxResult sandbox_result;
    // Note: sandbox_run now returns 0 always (to continue pipeline on failure)
    // Check exit_code to see if sandbox succeeded
    sandbox_run(code, &sandbox_result);

    // Step 4: Call ai_get_feedback(code, metrics, sandbox_output) from ai_client.c → Socratic response from Gemini
    // Convert metrics to JSON string
    cJSON *metrics_json_for_ai = cJSON_CreateObject();
    cJSON_AddItemToObject(metrics_json_for_ai, "lines", cJSON_CreateNumber(metrics.lines));
    cJSON_AddItemToObject(metrics_json_for_ai, "functions", cJSON_CreateNumber(metrics.functions));
    cJSON_AddItemToObject(metrics_json_for_ai, "max_nesting", cJSON_CreateNumber(metrics.max_nesting_depth));
    float quality_score = 1.0f / (1.0f + (float)metrics.dangerous_patterns);
    cJSON_AddItemToObject(metrics_json_for_ai, "quality_score", cJSON_CreateNumber(quality_score));
    char *metrics_json_str = cJSON_PrintUnformatted(metrics_json_for_ai);
    cJSON_Delete(metrics_json_for_ai);

    char *ai_feedback = ai_get_feedback(code, metrics_json_str, sandbox_result.stderr_buf);
    free(metrics_json_str);

    // Store submission and analysis in database
    int user_id = -1;
    int submission_id = -1;
    int analysis_id = -1;

    // Get username from header (or use demo)
    const char *username_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Username");
    const char *username = username_header && strlen(username_header) > 0 ? username_header : "demo_user";

    // Get or create user
    if (database_get_user_by_username(username, &user_id) != 0) {
        // User not found, create new user
        if (database_create_user(username, &user_id) != 0) {
            fprintf(stderr, "WARNING: Failed to create/get user '%s'\n", username);
            // Continue anyway - we can still return analysis results
            user_id = 1; // fallback to demo user ID
        }
    }

    // Create submission
    if (database_create_submission(user_id, code, &submission_id) != 0) {
        fprintf(stderr, "WARNING: Failed to create submission for user %d\n", user_id);
        // Continue anyway
    }

    // Create analysis
    if (submission_id != -1 && database_create_analysis(submission_id, &metrics, &sandbox_result, ai_feedback, &analysis_id) != 0) {
        fprintf(stderr, "WARNING: Failed to create analysis for submission %d\n", submission_id);
        // Continue anyway
    }

    // Build the JSON response
    cJSON *response_json = cJSON_CreateObject();

    // Add metrics
    cJSON *metrics_json = cJSON_CreateObject();
    cJSON_AddItemToObject(metrics_json, "lines", cJSON_CreateNumber(metrics.lines));
    cJSON_AddItemToObject(metrics_json, "functions", cJSON_CreateNumber(metrics.functions));
    cJSON_AddItemToObject(metrics_json, "max_nesting", cJSON_CreateNumber(metrics.max_nesting_depth));
    // Calculate quality score: 1.0 / (1.0 + dangerous_patterns)
    float quality_score2 = 1.0f / (1.0f + (float)metrics.dangerous_patterns);
    cJSON_AddItemToObject(metrics_json, "quality_score", cJSON_CreateNumber(quality_score2));
    cJSON_AddItemToObject(response_json, "metrics", metrics_json);

    // Add sandbox results
    cJSON *sandbox_json = cJSON_CreateObject();
    cJSON_AddItemToObject(sandbox_json, "compiled", cJSON_CreateBool(sandbox_result.exit_code == 0));
    cJSON_AddItemToObject(sandbox_json, "output", cJSON_CreateString(sandbox_result.stdout_buf));
    cJSON_AddItemToObject(sandbox_json, "errors", cJSON_CreateString(sandbox_result.stderr_buf));
    cJSON_AddItemToObject(response_json, "sandbox", sandbox_json);

    // Add AI feedback
    cJSON_AddItemToObject(response_json, "ai_feedback", cJSON_CreateString(ai_feedback ? ai_feedback : "No API key configured"));

    // Convert to string
    char *response_str = cJSON_PrintUnformatted(response_json);
    cJSON_Delete(json);
    cJSON_Delete(response_json);
    free(ai_feedback);

    if (response_str == NULL) {
        const char *error = "{\"error\": \"Failed to build response\"}";
        return create_json_response(error, connection);
    }

    struct MHD_Response *response = create_json_response(response_str, connection);
    free(response_str);
    return response;
}

/**
 * @brief Handles the /api/history route (GET).
 * @param connection The connection handle.
 * @param url The requested URL.
 * @param method The HTTP method.
 * @return MHD response object.
 */
static struct MHD_Response *handle_history(struct MHD_Connection *connection,
                                           const char *url,
                                           const char *method,
                                           const char *post_data) {
    (void)url; // Unused parameter
    (void)post_data; // Unused parameter
    if (strcmp(method, "GET") != 0) {
        const char *error = "{\"error\": \"Method not allowed\"}";
        return create_json_response(error, connection);
    }

    // Get username from header (or use demo)
    const char *username_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Username");
    const char *username = username_header && strlen(username_header) > 0 ? username_header : "demo_user";

    // Get user ID
    int user_id = -1;
    if (database_get_user_by_username(username, &user_id) != 0) {
        // User not found, create new user (shouldn't happen for history, but just in case)
        if (database_create_user(username, &user_id) != 0) {
            fprintf(stderr, "WARNING: Failed to create/get user '%s' for history\n", username);
            user_id = 1; // fallback to demo user ID
        }
    }

    // Get history from database
    char **history_rows = NULL;
    int history_count = 0;
    int ret = database_get_history(user_id, 50, 0, &history_rows, &history_count); // limit 50, offset 0
    if (ret != 0) {
        fprintf(stderr, "WARNING: Failed to get history for user %d\n", user_id);
        history_count = 0;
        history_rows = NULL;
    }

    // Build JSON response
    cJSON *response_json = cJSON_CreateObject();
    cJSON *history_array = cJSON_CreateArray();
    cJSON_AddItemToObject(response_json, "history", history_array);

    for (int i = 0; i < history_count; i++) {
        if (history_rows[i]) {
            cJSON *history_item = cJSON_Parse(history_rows[i]);
            if (history_item) {
                cJSON_AddItemToArray(history_array, history_item);
            }
            // Note: We don't free history_item here because it's now owned by the array
        }
    }

    char *response_str = cJSON_PrintUnformatted(response_json);
    cJSON_Delete(response_json);

    // Free history rows
    if (history_rows) {
        database_free_history(history_rows, history_count);
    }

    if (response_str == NULL) {
        const char *error = "{\"error\": \"Failed to build history response\"}";
        return create_json_response(error, connection);
    }

    struct MHD_Response *response = create_json_response(response_str, connection);
    free(response_str);
    return response;
}

/**
 * @brief Handles the /api/user/:id route (GET).
 * @param connection The connection handle.
 * @param url The requested URL.
 * @param method The HTTP method.
 * @return MHD response object.
 */
static struct MHD_Response *handle_user(struct MHD_Connection *connection,
                                        const char *url,
                                        const char *method,
                                        const char *post_data) {
    (void)post_data; // Unused parameter
    if (strcmp(method, "GET") != 0) {
        const char *error = "{\"error\": \"Method not allowed\"}";
        return create_json_response(error, connection);
    }

    // Extract user ID from URL: /api/user/:id
    if (strncmp(url, "/api/user/", 10) != 0) {
        const char *error = "{\"error\": \"Invalid user URL\"}";
        return create_json_response(error, connection);
    }
    const char *id_str = url + 10; // Skip "/api/user/"
    char *endptr;
    long user_id_long = strtol(id_str, &endptr, 10);
    if (endptr == id_str || *endptr != '\0' || user_id_long < 0 || user_id_long > INT_MAX) {
        const char *error = "{\"error\": \"Invalid user ID\"}";
        return create_json_response(error, connection);
    }
    int user_id = (int)user_id_long;

    // Fetch user data from database
    // We'll return a simple user object for now
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddItemToObject(response_json, "id", cJSON_CreateNumber(user_id));

    // Try to get username from database
    char username[256] = {0};
    // We don't have a direct function to get username by ID, so we'll just use a placeholder
    // In a real implementation, we would add a database_get_username_by_id function
    snprintf(username, sizeof(username), "user_%d", user_id);
    cJSON_AddItemToObject(response_json, "username", cJSON_CreateString(username));

    // We could also add created_at, etc. if we had those functions

    char *response_str = cJSON_PrintUnformatted(response_json);
    cJSON_Delete(response_json);

    if (response_str == NULL) {
        const char *error = "{\"error\": \"Failed to build user response\"}";
        return create_json_response(error, connection);
    }

    struct MHD_Response *response = create_json_response(response_str, connection);
    free(response_str);
    return response;
}

/**
 * @brief Request completed callback to free connection context.
 * @param cls Closure (unused).
 * @param connection The connection handle.
 * @param con_cls Pointer to connection-specific closure.
 * @param toe Termination code (unused).
 */
static void request_completed(void *cls, struct MHD_Connection *connection,
                              void **con_cls, enum MHD_RequestTerminationCode toe) {
    (void)cls; (void)connection; (void)toe;
    struct ConnectionContext *ctx = *con_cls;
    if (ctx) {
        free(ctx->post_data);
        free(ctx);
        *con_cls = NULL;
    }
}

/**
 * @brief Access callback for libmicrohttpd.
 * Dispatches to appropriate handler based on URL and method.
 * @param cls Closure (unused).
 * @param connection The connection handle.
 * @param url The requested URL.
 * @param method The HTTP method.
 * @param version The HTTP version (unused).
 * @param upload_data Upload data (unused).
 * @param upload_data_size Size of upload data (unused).
 * @param con_cls Pointer to connection-specific closure (unused).
 * @return MHD result code.
 */
static enum MHD_Result access_handler(void *cls,
                                      struct MHD_Connection *connection,
                                      const char *url,
                                      const char *method,
                                      const char *version,
                                      const char *upload_data,
                                      size_t *upload_data_size,
                                      void **con_cls) {
    (void)cls;
    (void)version;

    struct MHD_Response *response = NULL;
    int ret;

    if (*con_cls == NULL) {
        struct ConnectionContext *ctx = calloc(1, sizeof(struct ConnectionContext));
        if (ctx == NULL) return MHD_NO;
        *con_cls = ctx;
        return MHD_YES;
    }

    struct ConnectionContext *ctx = *con_cls;

    if (*upload_data_size > 0) {
        char *new_data = realloc(ctx->post_data, ctx->post_data_size + *upload_data_size + 1);
        if (new_data == NULL) return MHD_NO;
        ctx->post_data = new_data;
        memcpy(ctx->post_data + ctx->post_data_size, upload_data, *upload_data_size);
        ctx->post_data_size += *upload_data_size;
        ctx->post_data[ctx->post_data_size] = '\0';
        *upload_data_size = 0;
        return MHD_YES;
    }

    const char *post_data = ctx->post_data ? ctx->post_data : "";

    if (strcmp(url, "/api/analyze") == 0) {
        response = handle_analyze(connection, url, method, post_data);
    } else if (strcmp(url, "/api/history") == 0) {
        response = handle_history(connection, url, method, post_data);
    } else if (strncmp(url, "/api/user/", 10) == 0) {
        response = handle_user(connection, url, method, post_data);
    } else {
        const char *error = "{\"error\": \"Not found\"}";
        response = create_json_response(error, connection);
    }

    // Clean up connection context
    free(ctx->post_data);
    free(ctx);
    *con_cls = NULL;

    if (response == NULL) {
        return MHD_NO;
    }

    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

/**
 * @see server.h
 */
struct HTTPServer *server_create(const char *port) {
    struct HTTPServer *server = calloc(1, sizeof(struct HTTPServer));
    if (server == NULL) {
        return NULL;
    }
    server->port = port;
    return server;
}

/**
 * @see server.h
 */
int server_start(struct HTTPServer *server) {
    if (server == NULL) {
        return -1;
    }

    server->daemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
                                      atoi(server->port),
                                      NULL, NULL,
                                      &access_handler,
                                      NULL,
                                      MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
                                      MHD_OPTION_END);
    if (server->daemon == NULL) {
        return -1;
    }
    printf("MHD_start_daemon returned %p\n", (void*)server->daemon);
    return 0;
}

/**
 * @see server.h
 */
void server_stop(struct HTTPServer *server) {
    if (server != NULL && server->daemon != NULL) {
        MHD_stop_daemon(server->daemon);
        server->daemon = NULL;
    }
}

/**
 * @see server.h
 */
void server_destroy(struct HTTPServer *server) {
    if (server != NULL) {
        free(server);
    }
}