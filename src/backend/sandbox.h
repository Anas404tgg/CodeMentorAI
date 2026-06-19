#ifndef SANDBOX_H
#define SANDBOX_H

#include <stdio.h>

/**
 * @brief Result of sandbox execution.
 */
typedef struct {
    int exit_code;          ///< Exit code of the compiled program (0 for success)
    int timed_out;          ///< 1 if execution exceeded time limit, else 0
    char stdout_buf[4096];  ///< Captured standard output (null-terminated)
    char stderr_buf[4096];  ///< Captured standard error (null-terminated)
} SandboxResult;

/**
 * @brief Compiles and runs C source code in a sandboxed environment.
 *        Uses fork/execve, SIGALRM timeout (5 seconds), and ulimit for memory.
 * @param source Null-terminated C source code string.
 * @param result Pointer to SandboxResult to store output.
 * @return 0 on success (sandbox ran), -1 on failure to fork/exec.
 */
int sandbox_run(const char *source, SandboxResult *result);

#endif // SANDBOX_H