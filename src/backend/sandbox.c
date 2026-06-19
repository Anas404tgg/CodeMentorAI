/**
 * @file sandbox.c
 * @brief Lightweight sandbox for compiling and executing C code.
 *        Windows-only version using _popen() for simplicity.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sandbox.h"

#ifdef _WIN32
/* Windows-specific headers */
#include <windows.h>
#include <time.h>
#else
/* POSIX-specific headers */
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <errno.h>
#endif

/**
 * @brief Writes source code to a temporary file and returns the filename.
 *        Caller must free the returned string.
 * @param source C source code.
 * @return malloc'd filename or NULL on failure.
 */
static char *write_source_to_temp(const char *source) {
#ifdef _WIN32
    // Windows implementation - create temporary file in temp directory
    char temp_path[MAX_PATH];
    char temp_filename[MAX_PATH];

    // Get temp path
    DWORD temp_path_len = GetTempPathA(MAX_PATH, temp_path);
    if (temp_path_len == 0 || temp_path_len > MAX_PATH) {
        // Fallback to hardcoded path if GetTempPath fails
        strcpy_s(temp_path, sizeof(temp_path), "C:/Users/HP/Desktop/tidada/tmp/");
    }

    // Create temporary filename
    if (GetTempFileNameA(temp_path, "cod", 0, temp_filename) == 0) {
        // Fallback if GetTempFileName fails
        strcpy_s(temp_filename, sizeof(temp_filename), "C:/Users/HP/Desktop/tidada/tmp/temp_code.c");
        // Make it somewhat unique by adding a timestamp
        char timestamp[32];
        snprintf(timestamp, sizeof(timestamp), "%ld", (long)time(NULL));
        char *dot = strrchr(temp_filename, '.');
        if (dot) {
            memmove(dot + strlen(timestamp) + 1, dot, strlen(dot) + 1);
            memcpy(dot, "_", 1);
            memcpy(dot + 1, timestamp, strlen(timestamp));
        }
    }

    // Create the file
    HANDLE hFile = CreateFileA(
        temp_filename,
        GENERIC_WRITE,
        0,              // no sharing
        NULL,           // default security
        CREATE_ALWAYS,  // overwrite if exists
        FILE_ATTRIBUTE_NORMAL,
        NULL);          // no template

    if (hFile == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    // Write the source code
    DWORD bytes_written;
    BOOL write_result = WriteFile(
        hFile,
        source,
        (DWORD)strlen(source),
        &bytes_written,
        NULL);          // no overlapped structure

    CloseHandle(hFile);

    if (!write_result || bytes_written != strlen(source)) {
        return NULL;
    }

    // Return the filename (caller must free)
    return _strdup(temp_filename);
#else
    // This should not be used in our current Windows-only build, but keep for completeness
    char template[] = "/tmp/codememor_XXXXXX.c";
    int fd = mkstemp(template);
    if (fd == -1) {
        return NULL;
    }
    ssize_t len = write(fd, source, strlen(source));
    if (len != (ssize_t)strlen(source)) {
        close(fd);
        unlink(template);
        return NULL;
    }
    close(fd);
    // Return the filename (caller must free)
    return strdup(template);
#endif
}

/**
 * @brief Compiles a C source file to a temporary binary.
 * @param src_filename Source file path.
 * @param out_binary Pointer to buffer to receive binary path (must be freed).
 * @param err_buf Buffer to capture gcc stderr.
 * @param err_buf_size Size of err_buf.
 * @return 0 on success, -1 on failure.
 */
static int compile_source(const char *src_filename, char **out_binary,
                          char *err_buf, size_t err_buf_size) {
#ifdef _WIN32
    // Windows implementation - use CreateProcess for better control
    char binary_path[MAX_PATH];

    // Get temp path for binary
    char temp_path[MAX_PATH];
    DWORD temp_path_len = GetTempPathA(MAX_PATH, temp_path);
    if (temp_path_len == 0 || temp_path_len > MAX_PATH) {
        // Fallback to hardcoded path if GetTempPath fails
        strcpy_s(temp_path, sizeof(temp_path), "C:/Users/HP/Desktop/tidada/tmp/");
    }

    // Create temporary binary name
    if (GetTempFileNameA(temp_path, "cod", 0, binary_path) == 0) {
        // Fallback if GetTempFileName fails
        strcpy_s(binary_path, sizeof(binary_path), "C:/Users/HP/Desktop/tidada/tmp/temp_out.exe");
    }

    // Build the command: gcc -o <output> <input>
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "C:/msys64/mingw64/bin/gcc.exe -o \"%s\" \"%s\"", binary_path, src_filename);

    // Set up security attributes for inheriting handles
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create pipes for child's STDERR
    HANDLE hChildStdoutRd = NULL;
    HANDLE hChildStdoutWr = NULL;
    HANDLE hChildStderrRd = NULL;
    HANDLE hChildStderrWr = NULL;

    if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0) ||
        !CreatePipe(&hChildStderrRd, &hChildStderrWr, &saAttr, 0)) {
        // Clean up handles
        if (hChildStdoutRd) CloseHandle(hChildStdoutRd);
        if (hChildStdoutWr) CloseHandle(hChildStdoutWr);
        if (hChildStderrRd) CloseHandle(hChildStderrRd);
        if (hChildStderrWr) CloseHandle(hChildStderrWr);
        return -1;
    }

    // Ensure the read handles to the pipe are not inherited
    SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hChildStderrRd, HANDLE_FLAG_INHERIT, 0);

    // Set up STARTUPINFO structure
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOA siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
    siStartInfo.cb = sizeof(STARTUPINFOA);
    siStartInfo.hStdError = hChildStderrWr;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process
    BOOL bSuccess = CreateProcessA(
        NULL,               // No module name (use command line)
        cmd,                // Command line
        NULL,               // Process handle not inheritable
        NULL,               // Thread handle not inheritable
        TRUE,               // Set handle inheritance to TRUE
        0,                  // No creation flags
        NULL,               // Use parent's environment block
        NULL,               // Use parent's starting directory
        &siStartInfo,       // Pointer to STARTUPINFO structure
        &piProcInfo         // Pointer to PROCESS_INFORMATION structure
    );

    // Close pipe handles that were inherited by the child
    CloseHandle(hChildStdoutWr);
    CloseHandle(hChildStderrWr);

    if (!bSuccess) {
        // Clean up handles
        CloseHandle(hChildStdoutRd);
        CloseHandle(hChildStderrRd);
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        return -1;
    }

    // Wait for the process to finish or timeout (5 seconds)
    DWORD dwWaitResult = WaitForSingleObject(piProcInfo.hProcess, 5000); // 5 second timeout
    if (dwWaitResult == WAIT_TIMEOUT) {
        // Timeout exceeded, terminate the process
        TerminateProcess(piProcInfo.hProcess, 1);
    }

    // Read the process's output
    DWORD dwRead;
    CHAR chBuf[4096];
    BOOL bSuccessRead = FALSE;

    // Read from stderr pipe
    memset(err_buf, 0, err_buf_size);
    for (;;) {
        bSuccessRead = ReadFile(hChildStderrRd, chBuf, sizeof(chBuf) - 1, &dwRead, NULL);
        if (!bSuccessRead || dwRead == 0) break;

        chBuf[dwRead] = '\0';
        strncat_s(err_buf, err_buf_size, chBuf, err_buf_size - strlen(err_buf) - 1);
    }

    // Wait for the process to fully exit
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);

    // Get exit code
    DWORD dwExitCode = 0;
    GetExitCodeProcess(piProcInfo.hProcess, &dwExitCode);

    // Clean up handles
    CloseHandle(hChildStdoutRd);
    CloseHandle(hChildStderrRd);
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    // Check if compilation succeeded
    if (dwExitCode != 0) {
        // Compilation failed - err_buf already contains gcc output
        return -1;
    }

    *out_binary = _strdup(binary_path);
    if (*out_binary == NULL) {
        return -1;
    }
    return 0;
#else
    // POSIX implementation with proper resource limits and timeout
    char template_bin[] = "/tmp/codememor_XXXXXX";
    int fd = mkstemp(template_bin);
    if (fd == -1) {
        return -1;
    }
    close(fd); // we just need the path, gcc will create the executable
    char *binary_path = strdup(template_bin);
    if (binary_path == NULL) {
        unlink(template_bin);
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        free(binary_path);
        return -1;
    }
    if (pid == 0) {
        // Child: set limits and exec gcc
        // Set CPU time limit to 5 seconds
        struct rlimit cpu_limit;
        cpu_limit.rlim_cur = 5;  // 5 seconds soft limit
        cpu_limit.rlim_max = 5;  // 5 seconds hard limit
        if (setrlimit(RLIMIT_CPU, &cpu_limit) != 0) {
            _exit(127);
        }

        // Set memory limit to 128 MB
        struct rlimit mem_limit;
        mem_limit.rlim_cur = 128 * 1024 * 1024;  // 128 MB
        mem_limit.rlim_max = 128 * 1024 * 1024;  // 128 MB
        if (setrlimit(RLIMIT_AS, &mem_limit) != 0) {
            _exit(127);
        }

        execlp("gcc", "gcc", src_filename, "-o", binary_path, (char *)NULL);
        _exit(127);
    }

    int status;
    // Wait for child with timeout protection
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        *out_binary = binary_path;
        return 0;
    } else {
        free(binary_path);
        unlink(template_bin);
        return -1;
    }
#endif
}

/**
 * @brief Runs a binary in a sandboxed environment, capturing stdout/stderr.
 * @param binary_path Path to executable.
 * @param result Pointer to SandboxResult to fill.
 * @return 0 on success, -1 on failure to fork.
 */
static int run_binary(const char *binary_path, SandboxResult *result) {
#ifdef _WIN32
    // Windows implementation - use CreateProcess for better control and timeout
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "\"%s\"", binary_path);

    // Set up security attributes for inheriting handles
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create pipes for child's STDOUT and STDERR
    HANDLE hChildStdoutRd = NULL;
    HANDLE hChildStdoutWr = NULL;
    HANDLE hChildStderrRd = NULL;
    HANDLE hChildStderrWr = NULL;

    if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0) ||
        !CreatePipe(&hChildStderrRd, &hChildStderrWr, &saAttr, 0)) {
        // Clean up handles
        if (hChildStdoutRd) CloseHandle(hChildStdoutRd);
        if (hChildStdoutWr) CloseHandle(hChildStdoutWr);
        if (hChildStderrRd) CloseHandle(hChildStderrRd);
        if (hChildStderrWr) CloseHandle(hChildStderrWr);
        return -1;
    }

    // Ensure the read handles to the pipe are not inherited
    SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hChildStderrRd, HANDLE_FLAG_INHERIT, 0);

    // Set up STARTUPINFO structure
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFOA siStartInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFOA));
    siStartInfo.cb = sizeof(STARTUPINFOA);
    siStartInfo.hStdError = hChildStderrWr;
    siStartInfo.hStdOutput = hChildStdoutWr;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process
    BOOL bSuccess = CreateProcessA(
        NULL,               // No module name (use command line)
        cmd,                // Command line
        NULL,               // Process handle not inheritable
        NULL,               // Thread handle not inheritable
        TRUE,               // Set handle inheritance to TRUE
        0,                  // No creation flags
        NULL,               // Use parent's environment block
        NULL,               // Use parent's starting directory
        &siStartInfo,       // Pointer to STARTUPINFO structure
        &piProcInfo         // Pointer to PROCESS_INFORMATION structure
    );

    // Close pipe handles that were inherited by the child
    CloseHandle(hChildStdoutWr);
    CloseHandle(hChildStderrWr);

    if (!bSuccess) {
        // Clean up handles
        CloseHandle(hChildStdoutRd);
        CloseHandle(hChildStderrRd);
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        return -1;
    }

    // Wait for the process to finish or timeout (5 seconds)
    DWORD dwWaitResult = WaitForSingleObject(piProcInfo.hProcess, 5000); // 5 second timeout
    if (dwWaitResult == WAIT_TIMEOUT) {
        // Timeout exceeded, terminate the process
        TerminateProcess(piProcInfo.hProcess, 1);
        result->timed_out = 1;
    } else {
        result->timed_out = 0;
    }

    // Read the process's output
    DWORD dwRead;
    CHAR chBuf[4096];
    BOOL bSuccessRead = FALSE;

    // Read from stdout pipe
    memset(result->stdout_buf, 0, sizeof(result->stdout_buf));
    for (;;) {
        bSuccessRead = ReadFile(hChildStdoutRd, chBuf, sizeof(chBuf) - 1, &dwRead, NULL);
        if (!bSuccessRead || dwRead == 0) break;

        chBuf[dwRead] = '\0';
        strncat(result->stdout_buf, chBuf, sizeof(result->stdout_buf) - strlen(result->stdout_buf) - 1);
    }

    // Read from stderr pipe
    memset(result->stderr_buf, 0, sizeof(result->stderr_buf));
    for (;;) {
        bSuccessRead = ReadFile(hChildStderrRd, chBuf, sizeof(chBuf) - 1, &dwRead, NULL);
        if (!bSuccessRead || dwRead == 0) break;

        chBuf[dwRead] = '\0';
        strncat(result->stderr_buf, chBuf, sizeof(result->stderr_buf) - strlen(result->stderr_buf) - 1);
    }

    // Wait for the process to fully exit (in case of timeout termination)
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);

    // Get exit code
    DWORD dwExitCode = 0;
    GetExitCodeProcess(piProcInfo.hProcess, &dwExitCode);
    result->exit_code = (int)dwExitCode;

    // Clean up handles
    CloseHandle(hChildStdoutRd);
    CloseHandle(hChildStderrRd);
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    return 0;
#else
    // POSIX implementation with SIGALRM timeout (5 seconds max)
    int stdout_pipe[2];
    int stderr_pipe[2];
    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        return -1;
    }
    if (pid == 0) {
        // Child: set limits, redirect pipes, exec binary
        // Set CPU time limit to 5 seconds
        struct rlimit cpu_limit;
        cpu_limit.rlim_cur = 5;  // 5 seconds soft limit
        cpu_limit.rlim_max = 5;  // 5 seconds hard limit
        if (setrlimit(RLIMIT_CPU, &cpu_limit) != 0) {
            _exit(127);
        }

        // Set memory limit to 128 MB
        struct rlimit mem_limit;
        mem_limit.rlim_cur = 128 * 1024 * 1024;  // 128 MB
        mem_limit.rlim_max = 128 * 1024 * 1024;  // 128 MB
        if (setrlimit(RLIMIT_AS, &mem_limit) != 0) {
            _exit(127);
        }

        // Redirect stdout and stderr to pipes
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);

        execl(binary_path, binary_path, (char *)NULL);
        _exit(127);
    }

    // Parent: close write ends
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // Set up SIGALRM handler to interrupt waitpid
    struct sigaction sa;
    sa.sa_handler = SIG_IGN; // Ignore SIGALRM to just interrupt waitpid
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    // Start 5-second timer
    alarm(5);

    // Wait for child
    int status;
    pid_t waited_pid = waitpid(pid, &status, 0);
    int alarm_triggered = 0;

    if (waited_pid == -1 && errno == EINTR) {
        // Alarm went off, child process exceeded time limit
        alarm_triggered = 1;
        // Kill the child process
        kill(pid, SIGKILL);
        // Wait for it to actually terminate
        waitpid(pid, &status, 0);
    }

    // Disable any pending alarm
    alarm(0);

    // Read from pipes
    ssize_t n;
    n = read(stdout_pipe[0], result->stdout_buf, sizeof(result->stdout_buf) - 1);
    if (n > 0) {
        result->stdout_buf[n] = '\0';
    } else {
        result->stdout_buf[0] = '\0';
    }
    n = read(stderr_pipe[0], result->stderr_buf, sizeof(result->stderr_buf) - 1);
    if (n > 0) {
        result->stderr_buf[n] = '\0';
    } else {
        result->stderr_buf[0] = '\0';
    }

    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    if (WIFEXITED(status)) {
        result->exit_code = WEXITSTATUS(status);
        result->timed_out = 0;
    } else if (WIFSIGNALED(status)) {
        // Terminated by signal
        result->exit_code = 128 + WTERMSIG(status);
        result->timed_out = alarm_triggered ? 1 : 0;
    } else {
        result->exit_code = 1;
        result->timed_out = 0;
    }

    return 0;
#endif
}

int sandbox_run(const char *source, SandboxResult *result) {
    if (source == NULL || result == NULL) {
        // Set sandbox unavailable and return 0 to continue pipeline
        result->exit_code = -1;
        result->timed_out = 0;
        result->stdout_buf[0] = '\0';
        snprintf(result->stderr_buf, sizeof(result->stderr_buf), "Sandbox unavailable");
        return 0;
    }

    // Initialize result
    result->exit_code = 0;
    result->timed_out = 0;
    result->stdout_buf[0] = '\0';
    result->stderr_buf[0] = '\0';

    char *src_file = NULL;
    char *binary_path = NULL;
    int ret = -1;

    src_file = write_source_to_temp(source);
    if (src_file == NULL) {
        // Set sandbox unavailable and return 0 to continue pipeline
        result->exit_code = -1;
        result->timed_out = 0;
        result->stdout_buf[0] = '\0';
        snprintf(result->stderr_buf, sizeof(result->stderr_buf), "Sandbox unavailable");
        goto cleanup;
    }

    if (compile_source(src_file, &binary_path, result->stderr_buf, sizeof(result->stderr_buf)) != 0) {
        // Compilation failed - set sandbox unavailable and return 0 to continue pipeline
        result->exit_code = -1;
        result->timed_out = 0;
        result->stdout_buf[0] = '\0';
        snprintf(result->stderr_buf, sizeof(result->stderr_buf), "Sandbox unavailable");
        goto cleanup;
    }

    if (run_binary(binary_path, result) != 0) {
        // Failed to run binary - set sandbox unavailable and return 0 to continue pipeline
        result->exit_code = -1;
        result->timed_out = 0;
        result->stdout_buf[0] = '\0';
        snprintf(result->stderr_buf, sizeof(result->stderr_buf), "Sandbox unavailable");
        goto cleanup;
    }

    // Success
    ret = 0;

cleanup:
    if (src_file) {
#ifdef _WIN32
        DeleteFileA(src_file);
#else
        unlink(src_file);
#endif
        free(src_file);
    }
    if (binary_path) {
#ifdef _WIN32
        DeleteFileA(binary_path);
#else
        unlink(binary_path);
#endif
        free(binary_path);
    }
    return ret;
}