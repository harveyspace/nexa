/*
 * process_isolate.cpp — Child process management.
 *
 * On Linux/macOS: fork + exec nexa_child, communicate via Unix pipes.
 * On Windows: CreateProcess + named pipe (TODO).
 *
 * The child reads JSON commands from stdin, writes JSON responses to stdout.
 * stderr is captured for crash diagnostics.
 */
#include "process_isolate.h"
#include "ipc_protocol.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

struct ProcessIsolate {
    pid_t    pid;
    int      stdin_fd;    /* write to child */
    int      stdout_fd;   /* read from child */
    int      stderr_fd;   /* child stderr (for crash logs) */
    FILE*    in;
    FILE*    out;
    int      alive;
    int      exit_code;
    uint32_t next_msg_id;
};

/* ── Helpers ──────────────────────────────────────── */

static uint32_t read_u32_fd(int fd) {
    uint8_t buf[4];
    size_t total = 0;
    while (total < 4) {
        ssize_t n = read(fd, buf + total, 4 - total);
        if (n <= 0) return 0;
        total += n;
    }
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
         | ((uint32_t)buf[2] << 8)  |  (uint32_t)buf[3];
}

static void write_u32_fd(int fd, uint32_t v) {
    uint8_t buf[4] = {
        (uint8_t)(v >> 24), (uint8_t)(v >> 16),
        (uint8_t)(v >> 8),  (uint8_t)(v)
    };
    write(fd, buf, 4);
}

static char* read_response(int fd, int timeout_ms) {
    /* TODO: non-blocking with poll/select for timeout */
    uint32_t len = read_u32_fd(fd);
    if (len == 0 || len > 4*1024*1024) return nullptr;
    char* buf = (char*)malloc(len + 1);
    size_t total = 0;
    while (total < len) {
        ssize_t n = read(fd, buf + total, len - total);
        if (n <= 0) { free(buf); return nullptr; }
        total += n;
    }
    buf[len] = '\0';
    return buf;
}

static int check_json_ok(const char* json) {
    return strstr(json, "\"ok\":true") != nullptr;
}

/* ── Spawn ────────────────────────────────────────── */

ProcessIsolate* nexa_process_spawn(const char* child_binary_path,
                                    size_t heap_mb, size_t stack_kb) {
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdin_pipe)  < 0) return nullptr;
    if (pipe(stdout_pipe) < 0) { close(stdin_pipe[0]); close(stdin_pipe[1]); return nullptr; }
    if (pipe(stderr_pipe) < 0) {
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]); return nullptr;
    }

    pid_t pid = fork();
    if (pid < 0) return nullptr;

    if (pid == 0) {
        /* ── Child ── */
        dup2(stdin_pipe[0],  STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdin_pipe[0]);  close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);

        char heap_arg[32], stack_arg[32];
        snprintf(heap_arg, sizeof(heap_arg), "%zu", heap_mb);
        snprintf(stack_arg, sizeof(stack_arg), "%zu", stack_kb);

        execl(child_binary_path, child_binary_path,
              "--heap", heap_arg, "--stack", stack_arg, nullptr);
        _exit(127); /* exec failed */
    }

    /* ── Parent ── */
    close(stdin_pipe[0]);   /* we write to child stdin */
    close(stdout_pipe[1]);  /* we read from child stdout */
    close(stderr_pipe[1]);  /* we read from child stderr */

    ProcessIsolate* proc = (ProcessIsolate*)calloc(1, sizeof(ProcessIsolate));
    proc->pid       = pid;
    proc->stdin_fd  = stdin_pipe[1];
    proc->stdout_fd = stdout_pipe[0];
    proc->stderr_fd = stderr_pipe[0];
    proc->alive     = 1;
    return proc;
}

void nexa_process_kill(ProcessIsolate* proc) {
    if (!proc || !proc->alive) return;
    /* Send shutdown message first, then SIGTERM */
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "{\"id\":%u,\"method\":\"shutdown\",\"params\":{}}", ++proc->next_msg_id);
    write_u32_fd(proc->stdin_fd, (uint32_t)strlen(cmd));
    write(proc->stdin_fd, cmd, strlen(cmd));

    /* Wait briefly then force kill */
    usleep(100000); /* 100ms grace */
    kill(proc->pid, SIGTERM);
    waitpid(proc->pid, &proc->exit_code, WNOHANG);
    close(proc->stdin_fd);
    close(proc->stdout_fd);
    close(proc->stderr_fd);
    proc->alive = 0;
}

int nexa_process_alive(ProcessIsolate* proc) {
    if (!proc || !proc->alive) return 0;
    int status;
    pid_t result = waitpid(proc->pid, &status, WNOHANG);
    if (result == 0) return 1;
    if (result == proc->pid) {
        proc->alive = 0;
        proc->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
    return proc->alive;
}

int nexa_process_pid(ProcessIsolate* proc) {
    return proc ? (int)proc->pid : -1;
}

/* ── Send command ─────────────────────────────────── */

char* nexa_process_send(ProcessIsolate* proc, const char* method_json,
                         int timeout_ms) {
    if (!proc || !proc->alive) return nullptr;
    (void)timeout_ms; /* TODO */

    /* Send */
    uint32_t len = (uint32_t)strlen(method_json);
    write_u32_fd(proc->stdin_fd, len);
    if (write(proc->stdin_fd, method_json, len) != (ssize_t)len) {
        proc->alive = 0;
        return nullptr;
    }

    /* Receive */
    char* response = read_response(proc->stdout_fd, timeout_ms);
    if (!response) {
        proc->alive = 0;
    }
    return response;
}

/* ── Convenience wrappers ─────────────────────────── */

static char* make_cmd(const char* method, const char* params, uint32_t* id_counter) {
    static char buf[4096];
    uint32_t id = ++(*id_counter);
    snprintf(buf, sizeof(buf),
        "{\"id\":%u,\"method\":\"%s\",\"params\":%s}", id, method, params);
    return buf;
}

char* nexa_process_run(ProcessIsolate* proc, const char* script,
                        int timeout_ms) {
    char params[4096];
    /* Escape script for JSON */
    snprintf(params, sizeof(params), "{\"script\":\"%s\"}", script);
    return nexa_process_send(proc,
        make_cmd("run", params, &proc->next_msg_id), timeout_ms);
}

char* nexa_process_call(ProcessIsolate* proc, const char* func,
                         const char* json_args, int timeout_ms) {
    char params[4096];
    snprintf(params, sizeof(params),
        "{\"function\":\"%s\",\"args\":\"%s\"}", func, json_args);
    return nexa_process_send(proc,
        make_cmd("call", params, &proc->next_msg_id), timeout_ms);
}

char* nexa_process_load(ProcessIsolate* proc, const char* name,
                         const char* script, int timeout_ms) {
    char params[4096];
    snprintf(params, sizeof(params),
        "{\"name\":\"%s\",\"script\":\"%s\"}", name, script);
    return nexa_process_send(proc,
        make_cmd("load", params, &proc->next_msg_id), timeout_ms);
}

char* nexa_process_gc(ProcessIsolate* proc) {
    return nexa_process_send(proc,
        make_cmd("gc", "{}", &proc->next_msg_id), 1000);
}

char* nexa_process_stats(ProcessIsolate* proc) {
    return nexa_process_send(proc,
        make_cmd("stats", "{}", &proc->next_msg_id), 1000);
}
