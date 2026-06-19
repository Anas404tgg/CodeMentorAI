/**
 * @file test_parser.c
 * @brief Unit tests for the C static parser.
 *
 * Tests the parse_c_code function with various inputs and checks the metrics.
 */

#include <stdio.h>
#include <string.h>
#include "parser.h"

/**
 * @brief Runs a single test case.
 * @param description Description of the test.
 * @param code The C code to parse.
 * @param expected_lines Expected lines metric.
 * @param expected_functions Expected functions metric.
 * @param expected_max_nesting_depth Expected max nesting depth.
 * @param expected_dangerous_patterns Expected dangerous patterns count.
 * @return 0 on success, -1 on failure.
 */
static int run_test(const char *description,
                    const char *code,
                    int expected_lines,
                    int expected_functions,
                    int expected_max_nesting_depth,
                    int expected_dangerous_patterns) {
    Metrics metrics;
    int ret = parse_c_code(code, &metrics);
    if (ret != 0) {
        printf("FAIL: %s - parse_c_code returned %d\n", description, ret);
        return -1;
    }

    int success = 1;
    if (metrics.lines != expected_lines) {
        printf("FAIL: %s - lines: expected %d, got %d\n",
               description, expected_lines, metrics.lines);
        success = 0;
    }
    if (metrics.functions != expected_functions) {
        printf("FAIL: %s - functions: expected %d, got %d\n",
               description, expected_functions, metrics.functions);
        success = 0;
    }
    if (metrics.max_nesting_depth != expected_max_nesting_depth) {
        printf("FAIL: %s - max_nesting_depth: expected %d, got %d\n",
               description, expected_max_nesting_depth, metrics.max_nesting_depth);
        success = 0;
    }
    if (metrics.dangerous_patterns != expected_dangerous_patterns) {
        printf("FAIL: %s - dangerous_patterns: expected %d, got %d\n",
               description, expected_dangerous_patterns, metrics.dangerous_patterns);
        success = 0;
    }

    if (success) {
        printf("PASS: %s\n", description);
        return 0;
    } else {
        return -1;
    }
}

int main(void) {
    int failed = 0;

    // Test 1: Empty string
    failed += run_test("Empty string", "", 0, 0, 0, 0);

    // Test 2: Single line with no newline
    failed += run_test("Single line without newline", "int main(){}", 1, 1, 1, 0);

    // Test 3: Single line with newline
    failed += run_test("Single line with newline", "int main(){}\n", 2, 1, 1, 0);

    // Test 4: Function with nested braces
    failed += run_test("Nested braces", "int main() { if (1) { while (1) { break; } } }", 1, 1, 3, 0);

    // Test 5: Multiple functions
    failed += run_test("Multiple functions", "int a() {}\nint b() {}\n", 2, 2, 1, 0);

    // Test 6: Dangerous pattern - gets
    failed += run_test("Dangerous gets", "void f() { gets(buf); }", 1, 1, 1, 1);

    // Test 7: Multiple dangerous patterns
    failed += run_test("Multiple dangerous patterns", "void f() { gets(buf); strcpy(dest, src); }", 1, 1, 1, 2);

    // Test 8: Function with parameters and return
    failed += run_test("Function with parameters", "int add(int a, int b) { return a + b; }", 1, 1, 1, 0);

    // Test 9: Preprocessor directives (should be ignored for our simple parser)
    failed += run_test("Preprocessor", "#include <stdio.h>\n#define MAX 100\nint main() { printf(\"%d\\n\", MAX); }", 3, 1, 1, 0);

    // Test 10: String literals that look like dangerous patterns (should not be counted)
    failed += run_test("String literal with gets", "void f() { char *s = \"gets(\"; }", 1, 1, 1, 0);

    // Test 11: Comment that looks like dangerous pattern
    failed += run_test("Comment with gets", "void f() { /* gets(buf) */ }", 1, 1, 1, 0);

    if (failed == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed.\n", failed);
        return -1;
    }
}