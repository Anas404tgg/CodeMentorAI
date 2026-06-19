#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <cjson/cJSON.h>

/**
 * @brief Creates a JSON object from the given key-value pair (string, string).
 *        Returns a newly allocated cJSON object that must be freed with cJSON_Delete.
 * @param key The key (must not be NULL).
 * @param value The value (must not be NULL).
 * @return Pointer to a cJSON object, or NULL on failure.
 */
cJSON *json_create_string_string(const char *key, const char *value);

/**
 * @brief Creates a JSON object from the given key-value pair (string, int).
 *        Returns a newly allocated cJSON object that must be freed with cJSON_Delete.
 * @param key The key (must not be NULL).
 * @param value The integer value.
 * @return Pointer to a cJSON object, or NULL on failure.
 */
cJSON *json_create_string_int(const char *key, int value);

/**
 * @brief Extracts a string value from a cJSON object by key.
 *        Returns a newly allocated string that must be freed with free, or NULL if not found or not a string.
 * @param object The cJSON object.
 * @param key The key to look for.
 * @return Dynamically allocated string, or NULL.
 */
char *json_get_string_value(cJSON *object, const char *key);

/**
 * @brief Extracts an integer value from a cJSON object by key.
 *        Returns the integer value, or 0 if not found or not an integer.
 * @param object The cJSON object.
 * @param key The key to look for.
 * @return Integer value.
 */
int json_get_int_value(cJSON *object, const char *key);

#endif // JSON_UTILS_H