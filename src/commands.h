/* TinyOS V1 — Shell Built-in Commands
 *
 * Each command handler receives (argc, argv) where argv[0] is the
 * command name and argv[1..argc-1] are the arguments.
 *
 * Handlers return EFI_SUCCESS on success.
 */

#ifndef TINYOS_COMMANDS_H
#define TINYOS_COMMANDS_H

#include "uefi/types.h"

/* Shell command function signature */
typedef EFI_STATUS (*ShellCmdHandler)(int argc, char **argv);

/* Command table entry */
struct ShellCommand {
    const char      *name;
    const char      *help;
    ShellCmdHandler  handler;
};

/* The global command table — null-terminated */
extern ShellCommand g_cmd_table[];
extern int          g_cmd_count;

/* Built-in command handlers */
EFI_STATUS cmd_ls(int argc, char **argv);
EFI_STATUS cmd_cd(int argc, char **argv);
EFI_STATUS cmd_mkdir(int argc, char **argv);
EFI_STATUS cmd_help(int argc, char **argv);
EFI_STATUS cmd_clear(int argc, char **argv);
EFI_STATUS cmd_info(int argc, char **argv);
EFI_STATUS cmd_read(int argc, char **argv);
EFI_STATUS cmd_wallpaper(int argc, char **argv);
EFI_STATUS cmd_shutdown(int argc, char **argv);
EFI_STATUS cmd_pfls(int argc, char **argv);
EFI_STATUS cmd_mkfile(int argc, char **argv);
EFI_STATUS cmd_pfcat(int argc, char **argv);
EFI_STATUS cmd_pfrm(int argc, char **argv);
EFI_STATUS cmd_pfwrite(int argc, char **argv);
EFI_STATUS cmd_install(int argc, char **argv);

#endif /* TINYOS_COMMANDS_H */
