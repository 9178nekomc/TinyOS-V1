/* TinyOS V1 — Interactive Shell
 *
 * Provides the command-line REPL (Read-Eval-Print Loop).
 * Uses console_readline() for input (keyboard + serial fallback)
 * and dispatches to registered command handlers.
 */

#ifndef TINYOS_SHELL_H
#define TINYOS_SHELL_H

#include "uefi/types.h"

/* Initialize the shell — registers all built-in commands.
 * Called once at boot before shell_run(). */
void shell_init();

/* Enter the interactive REPL. Does not return. */
void shell_run();

/* Parse and execute a single command line.
 * Useful for programmatic command execution. */
EFI_STATUS shell_execute(char *line);

#endif /* TINYOS_SHELL_H */
