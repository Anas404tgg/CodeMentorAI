#ifndef SERVER_H
#define SERVER_H

#include <microhttpd.h>

/**
 * @brief Structure representing the HTTP server.
 */
struct HTTPServer {
    struct MHD_Daemon *daemon;
    const char *port;
};

/**
 * @brief Creates and initializes the HTTP server.
 * @param port The port number as a string (e.g., "8080").
 * @return Pointer to the server structure on success, NULL on failure.
 */
struct HTTPServer *server_create(const char *port);

/**
 * @brief Starts the HTTP server.
 * @param server Pointer to the server structure.
 * @return 0 on success, -1 on failure.
 */
int server_start(struct HTTPServer *server);

/**
 * @brief Stops the HTTP server.
 * @param server Pointer to the server structure.
 */
void server_stop(struct HTTPServer *server);

/**
 * @brief Frees resources associated with the server.
 * @param server Pointer to the server structure.
 */
void server_destroy(struct HTTPServer *server);

#endif // SERVER_H