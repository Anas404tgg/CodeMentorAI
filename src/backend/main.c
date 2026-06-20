/**
 * @file main.c
 * @brief Entry point of the CodeMentor AI backend.
 *
 * Initializes environment variables, starts the HTTP server,
 * and runs the main event loop.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <microhttpd.h>

#ifndef _WIN32
extern int putenv(char *string);
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include "server.h"      // To be created in module 2
#include "parser.h"      // To be created in module 3
#include "sandbox.h"     // To be created in module 4
#include "database.h"    // To be created in module 5
#include "ai_client.h"   // To be created in module 6
#include "json_utils.h"  // To be created in module 7

#define DEFAULT_PORT "8080"

/**
 * @brief Loads environment variables from a .env file.
 * @param filename Path to the .env file.
 */
static void load_env_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return; /* No .env file, continue with system environment */
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        /* Remove newline */
        line[strcspn(line, "\r\n")] = 0;

        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        /* Split on first '=' */
        char *eq = strchr(line, '=');
        if (!eq) {
            continue; /* Invalid line */
        }

        *eq = '\0';
        char *key = line;
        char *value = eq + 1;

        /* Set the environment variable (overwrites if exists) */
        char env_var[512]; // Increased buffer size to prevent truncation
        int len = snprintf(env_var, sizeof(env_var), "%s=%s", key, value);
        if (len >= 0 && len < (int)sizeof(env_var)) {
            #ifdef _WIN32
	_putenv(env_var);
#else
	putenv(env_var);
#endif
        }
        /* If truncation would occur, skip this environment variable */
    }

    fclose(file);
}

/**
 * @brief Loads configuration from environment variables.
 * @param[out] port String buffer to store the port number.
 * @param[in] size Size of the port buffer.
 */
static void load_config(char *port, size_t size) {
    const char *env_port = getenv("PORT");
    if (env_port != NULL && strlen(env_port) < size) {
        strcpy(port, env_port);
    } else {
        strcpy(port, DEFAULT_PORT);
    }
}

/**
 * @brief Main function: initializes modules and starts the server.
 * @return Exit status.
 */
int main(void) {
    char port[16];
    load_env_file(".env");
    load_config(port, sizeof(port));

    // Initialize subsystems
    if (database_init() != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return EXIT_FAILURE;
    }
    if (ai_client_init() != 0) {
        fprintf(stderr, "Failed to initialize AI client\n");
        database_cleanup();
        return EXIT_FAILURE;
    }

    // Start HTTP server
    struct HTTPServer *server = server_create(port);
    if (server == NULL) {
        fprintf(stderr, "Failed to create server\n");
        ai_client_cleanup();
        database_cleanup();
        return EXIT_FAILURE;
    }

    printf("CodeMentor AI backend listening on port %s\n", port);
    if (server_start(server) != 0) {
        fprintf(stderr, "Failed to start server\n");
        server_destroy(server);
        ai_client_cleanup();
        database_cleanup();
        return EXIT_FAILURE;
    }

    // Server runs until interrupted (e.g., SIGINT)
    // In a real application, we would wait for a shutdown signal.
    // For simplicity, we'll loop until getchar() (not suitable for production).
    // We'll implement proper signal handling in server.c.
    printf("Server running... Press Ctrl+C to stop.\n");
    while (1) {
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    // Cleanup
    server_stop(server);
    server_destroy(server);
    ai_client_cleanup();
    database_cleanup();

    return EXIT_SUCCESS;
}