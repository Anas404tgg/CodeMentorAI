/**
 * @file ai_client.c
 * @brief AI client implementation using libcurl to call Gemini API.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "ai_client.h"

/* Global variable to hold the API key */
static const char *api_key = NULL;

/* Structure for storing memory chunks during curl operations */
struct MemoryStruct {
    char *memory;
    size_t size;
};

/**
 * @brief Callback function for libcurl to write received data into a string.
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

/**
 * @brief Initializes the AI client by checking for the GEMINI_API_KEY environment variable.
 * @return 0 on success, -1 on failure.
 */
int ai_client_init(void) {
    api_key = getenv("GEMINI_API_KEY");
    if (api_key == NULL || strlen(api_key) == 0) {
        fprintf(stderr, "GEMINI_API_KEY environment variable not set or empty\n");
        return -1;
    }
    fprintf(stderr, "AI client initialized successfully with GEMINI_API_KEY\n");
    return 0;
}

/**
 * @brief Cleans up the AI client.
 */
void ai_client_cleanup(void) {
    /* Nothing to clean up for now */
    api_key = NULL;
}

/**
 * @brief Sends a prompt to the AI API and returns the response.
 *        The caller must free the returned string.
 * @param prompt The full prompt to send to the AI.
 * @return Dynamically allocated string containing the AI's response, or NULL on failure.
 */
char *ai_client_send_prompt(const char *prompt) {
    if (prompt == NULL) {
        fprintf(stderr, "AI client: NULL prompt provided\n");
        return NULL;
    }
    if (api_key == NULL) {
        fprintf(stderr, "AI client: No API key available\n");
        return NULL;
    }

    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        /* Construct the URL */
        char url[256];
        snprintf(url, sizeof(url), "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=%s", api_key);

        /* Construct the JSON payload */
        cJSON *root = cJSON_CreateObject();
        cJSON *contents = cJSON_CreateArray();
        cJSON *item = cJSON_CreateObject();
        cJSON *parts = cJSON_CreateArray();
        cJSON *part = cJSON_CreateObject();
        cJSON_AddItemToObject(part, "text", cJSON_CreateString(prompt));
        cJSON_AddItemToArray(parts, part);
        cJSON_AddItemToObject(item, "parts", parts);
        cJSON_AddItemToArray(contents, item);
        cJSON_AddItemToObject(root, "contents", contents);
        char *payload = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);

        /* Set up the request */
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(payload));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, "Content-Type: application/json"));

        /* Perform the request */
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "AI client: curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            chunk.memory = NULL;
        } else {
            /* Parse the response to extract the text */
            cJSON *response = cJSON_Parse(chunk.memory);
            if (response != NULL) {
                // Check for error object
                cJSON *error = cJSON_GetObjectItemCaseSensitive(response, "error");
                if (error != NULL && cJSON_IsObject(error)) {
                    cJSON *message = cJSON_GetObjectItemCaseSensitive(error, "message");
                    if (message != NULL && cJSON_IsString(message)) {
                        char *result = strdup(message->valuestring);
                        cJSON_Delete(response);
                        free(payload);
                        curl_easy_cleanup(curl);
                        curl_global_cleanup();
                        free(chunk.memory);
                        chunk.memory = NULL;
                        return result;
                    }
                }
                // If no error, then try to parse the success response
                if (response != NULL) {
                    cJSON *candidates = cJSON_GetObjectItemCaseSensitive(response, "candidates");
                    if (cJSON_IsArray(candidates) && cJSON_GetArraySize(candidates) > 0) {
                        cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
                        if (first_candidate != NULL) {
                            cJSON *content = cJSON_GetObjectItemCaseSensitive(first_candidate, "content");
                            if (content != NULL) {
                                cJSON *parts_array = cJSON_GetObjectItemCaseSensitive(content, "parts");
                                if (cJSON_IsArray(parts_array) && cJSON_GetArraySize(parts_array) > 0) {
                                    cJSON *first_part = cJSON_GetArrayItem(parts_array, 0);
                                    if (first_part != NULL) {
                                        cJSON *text = cJSON_GetObjectItemCaseSensitive(first_part, "text");
                                        if (text != NULL && cJSON_IsString(text)) {
                                            /* Duplicate the string to return */
                                            char *result = strdup(text->valuestring);
                                            cJSON_Delete(response);
                                            free(payload);
                                            curl_easy_cleanup(curl);
                                            curl_global_cleanup();
                                            return result;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                cJSON_Delete(response);
            }
            /* If we get here, we didn't find the expected text */
            fprintf(stderr, "AI client: Failed to parse expected text from Gemini response\n");
            free(chunk.memory);
            chunk.memory = NULL;
        }

        free(payload);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return chunk.memory; /* will be NULL if there was an error */
}

/**
 * @brief Gets AI feedback for code analysis using Gemini API.
 *        Constructs a Socratic prompt from code, metrics, and sandbox errors.
 *        The caller must free the returned string.
 * @param code The source code that was analyzed
 * @param metrics_json JSON string containing code metrics
 * @param sandbox_errors Errors from sandbox execution (compilation/runtime)
 * @return Dynamically allocated string containing AI feedback, or NULL on failure.
 */
char *ai_get_feedback(const char *code, const char *metrics_json, const char *sandbox_errors) {
    // Check if API key is available
    if (api_key == NULL || strlen(api_key) == 0) {
        return strdup("AI feedback unavailable - no API key");
    }

    // Validate input parameters
    if (code == NULL) {
        code = "";
    }
    if (metrics_json == NULL) {
        metrics_json = "{}";
    }
    if (sandbox_errors == NULL) {
        sandbox_errors = "";
    }

    // Construct the Socratic prompt in French as requested - exact format from ioo.txt
    const char *system_prompt = "Tu es un tuteur bienveillant en informatique à l'ESISA.\n"
                                "Un étudiant te soumet son code C et son erreur de compilation/exécution.\n"
                                "RÈGLES ABSOLUES :\n"
                                "1. Ne donne JAMAIS le code corrigé.\n"
                                "2. Explique la logique de l'erreur en termes simples.\n"
                                "3. Cite la ligne problématique probable.\n"
                                "4. Pose une question-guide pour que l'étudiant trouve lui-même.\n"
                                "5. Adapte ton niveau de langage à un étudiant de 1ère année.\n"
                                "Format de réponse :\n"
                                "- 🔍 Où est le problème\n"
                                "- 💡 Le concept à comprendre\n"
                                "- 🛠 Un indice sous forme de question";

    // Build the full prompt
    // We'll format it as: [system_prompt]\n\nCode:\n[code]\n\nMetrics:\n[metrics_json]\n\nErrors:\n[sandbox_errors]
    const char *format = "%s\n\nCode:\n%s\n\nMetrics:\n%s\n\nErrors:\n%s";

    // Calculate required buffer size
    int size_needed = snprintf(NULL, 0, format, system_prompt, code, metrics_json, sandbox_errors);
    if (size_needed < 0) {
        return NULL;
    }

    // Allocate buffer for the prompt
    char *prompt = malloc(size_needed + 1);
    if (prompt == NULL) {
        return NULL;
    }

    // Format the prompt
    snprintf(prompt, size_needed + 1, format, system_prompt, code, metrics_json, sandbox_errors);

    // Send the prompt to the AI API using existing function
    char *response = ai_client_send_prompt(prompt);

    // Free the prompt buffer
    free(prompt);

    return response;
}
