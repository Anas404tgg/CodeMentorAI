#ifndef PARSER_H
#define PARSER_H

/**
 * @brief Structure holding code metrics computed by the parser.
 */
typedef struct {
    int lines;           ///< Number of lines (count of newline characters + 1)
    int functions;       ///< Number of function definitions (heuristic: global scope opening braces)
    int max_nesting_depth; ///< Maximum nesting depth of curly braces
    int dangerous_patterns; ///< Count of dangerous function calls (gets, strcpy, etc.)
} Metrics;

/**
 * @brief Parses C source code and computes metrics.
 * @param code Null-terminated C source code string.
 * @param metrics Pointer to a Metrics structure to store results.
 * @return 0 on success, -1 if code or metrics is NULL.
 */
int parse_c_code(const char *code, Metrics *metrics);

#endif // PARSER_H