/**
 * @file json_utils.c
 * @brief Helper functions for cJSON to simplify JSON creation and parsing.
 */

#include <stdlib.h>
#include <string.h>
#include "json_utils.h"

cJSON *json_create_string_string(const char *key, const char *value) {
    if (key == NULL || value == NULL) {
        return NULL;
    }
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    cJSON_AddItemToObject(obj, key, cJSON_CreateString(value));
    return obj;
}

cJSON *json_create_string_int(const char *key, int value) {
    if (key == NULL) {
        return NULL;
    }
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    cJSON_AddItemToObject(obj, key, cJSON_CreateNumber(value));
    return obj;
}

char *json_get_string_value(cJSON *object, const char *key) {
    if (object == NULL || key == NULL) {
        return NULL;
    }
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        return strdup(item->valuestring);
    }
    return NULL;
}

int json_get_int_value(cJSON *object, const char *key) {
    if (object == NULL || key == NULL) {
        return 0;
    }
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (cJSON_IsNumber(item)) {
        return item->valueint;
    }
    return 0;
}