/**
 * @file parser.c
 * @brief Native C static parser (char-by-char) to compute code metrics.
 *
 * Computes lines, function definitions, maximum nesting depth of curly braces,
 * and counts dangerous function calls (gets, strcpy, etc.).
 */

#include <stdio.h>
#include <string.h>
#include "parser.h"

/**
 * @brief Helper to check if a substring matches at a given position.
 * @param str The string to search.
 * @param pos Starting position.
 * @param substr Substring to match.
 * @return 1 if match, 0 otherwise.
 */
static int match_substr(const char *str, size_t pos, const char *substr) {
    return strncmp(str + pos, substr, strlen(substr)) == 0;
}

/**
 * @brief Parses C source code and computes metrics.
 * @param code Null-terminated C source code string.
 * @param metrics Pointer to a Metrics structure to store results.
 * @return 0 on success, -1 if code or metrics is NULL.
 */
int parse_c_code(const char *code, Metrics *metrics) {
    if (code == NULL || metrics == NULL) {
        return -1;
    }

    // Initialize metrics
    metrics->lines = 1;           // at least one line if no newline
    metrics->functions = 0;
    metrics->max_nesting_depth = 0;
    metrics->dangerous_patterns = 0;

    int depth = 0;                // current nesting depth of curly braces
    size_t i = 0;
    size_t len = strlen(code);

    // Context tracking for ignoring dangerous patterns in strings/comments
    int in_string = 0;            // Are we inside a string literal?
    char string_delimiter = 0;    // What started the string: '"' or '''
    int in_block_comment = 0;     // Are we inside a /* ... */ comment?
    int in_line_comment = 0;      // Are we inside a // ... \n comment?

    while (i < len) {
        char ch = code[i];

        // Handle string literals
        if (!in_block_comment && !in_line_comment) {
            if ((ch == '"' || ch == '\'') && (i == 0 || code[i-1] != '\\')) {
                if (!in_string) {
                    // Starting a string literal
                    in_string = 1;
                    string_delimiter = ch;
                } else if (ch == string_delimiter) {
                    // Ending a string literal
                    in_string = 0;
                    string_delimiter = 0;
                }
            }
        }

        // Handle block comments
        if (!in_string && !in_line_comment) {
            if (match_substr(code, i, "/*")) {
                in_block_comment = 1;
                i++; // Skip the '*'
            } else if (match_substr(code, i, "*/") && in_block_comment) {
                in_block_comment = 0;
                i++; // Skip the '/'
            }
        }

        // Handle line comments
        if (!in_string && !in_block_comment) {
            if (match_substr(code, i, "//")) {
                in_line_comment = 1;
                i++; // Skip the second '/'
            }
        }

        // End line comment at newline
        if (ch == '\n' && in_line_comment) {
            in_line_comment = 0;
        }

        // Count lines
        if (ch == '\n') {
            metrics->lines++;
        }

        // Track nesting depth for braces
        if (ch == '{') {
            depth++;
            if (depth > metrics->max_nesting_depth) {
                metrics->max_nesting_depth = depth;
            }
            // Heuristic: a brace at depth 1 indicates start of a function body
            // (global scope depth 0, entering first brace -> function body)
            if (depth == 1) {
                metrics->functions++;
            }
        } else if (ch == '}') {
            if (depth > 0) {
                depth--;
            }
        }

        // Detect dangerous patterns (function calls) - ONLY when not in string/comment
        if (!in_string && !in_block_comment && !in_line_comment) {
            // We'll check for common unsafe functions
            if (match_substr(code, i, "gets(")) {
                metrics->dangerous_patterns++;
                i += strlen("gets(") - 1; // skip ahead to avoid multiple counts
            } else if (match_substr(code, i, "strcpy(")) {
                metrics->dangerous_patterns++;
                i += strlen("strcpy(") - 1;
            } else if (match_substr(code, i, "strcat(")) {
                metrics->dangerous_patterns++;
                i += strlen("strcat(") - 1;
            } else if (match_substr(code, i, "sprintf(")) {
                metrics->dangerous_patterns++;
                i += strlen("sprintf(") - 1;
            } else if (match_substr(code, i, "vsprintf(")) {
                metrics->dangerous_patterns++;
                i += strlen("vsprintf(") - 1;
            }
        }

        i++;
    }

    // Adjust lines: if file ends without newline, lines count is correct.
    // If empty string, lines should be 0.
    if (len == 0) {
        metrics->lines = 0;
    }

    return 0;
}