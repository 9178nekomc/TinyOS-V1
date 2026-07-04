/* TinyOS V1 — Shell REPL (Graphical Terminal Edition) */

#include "shell.h"
#include "commands.h"
#include "gfxterm.h"
#include "serial.h"
#include "alloc.h"
#include "fat.h"

#define SHELL_MAX_ARGS 16
#define SHELL_MAX_LINE 256

static char  g_line[SHELL_MAX_LINE];
static char *g_argv[SHELL_MAX_ARGS];
static int   g_argc = 0;

/* ===========================================================================
 * Tokenization
 * =========================================================================== */

static int shell_tokenize(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;
    while (*p && argc < max_args) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
        if (*p) { *p = '\0'; p++; }
    }
    argv[argc] = nullptr;
    return argc;
}

/* ===========================================================================
 * Command dispatch
 * =========================================================================== */

static int str_cmp(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (int)(*a) - (int)(*b);
}

EFI_STATUS shell_execute(char *line) {
    g_argc = shell_tokenize(line, g_argv, SHELL_MAX_ARGS);
    if (g_argc == 0) return EFI_SUCCESS;

    for (int i = 0; g_cmd_table[i].name != nullptr; i++) {
        if (str_cmp(g_argv[0], g_cmd_table[i].name) == 0) {
            return g_cmd_table[i].handler(g_argc, g_argv);
        }
    }

    gfxterm_print("Unknown command: '");
    gfxterm_print(g_argv[0]);
    gfxterm_print("'. Type 'help' for available commands.\n");
    return EFI_NOT_FOUND;
}

/* ===========================================================================
 * Init & REPL
 * =========================================================================== */

void shell_init() {
    serial_print("[SHELL] ");
    serial_print_dec(g_cmd_count);
    serial_print(" commands ready.\r\n");
}

/* Helper: print the shell prompt to both gfxterm and serial */
static void shell_prompt() {
    gfxterm_print("\ntinyos:");
    gfxterm_print(g_cwd);
    gfxterm_print("> ");
}

void shell_run() {
    while (1) {
        shell_prompt();
        int len = gfxterm_readline(g_line, SHELL_MAX_LINE);
        if (len == 0) continue;
        shell_execute(g_line);
    }
}
