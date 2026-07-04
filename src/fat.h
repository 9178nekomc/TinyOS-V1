/* TinyOS V1 — Filesystem Operations via EFI_FILE_PROTOCOL
 *
 * Provides directory listing (ls), directory navigation (cd), and
 * directory creation (mkdir) using the UEFI Simple File System Protocol.
 *
 * This operates on Disk 0 (the ESP virtual FAT drive), NOT on the
 * raw disk0.img (Disk 1). ATA PIO is only used for Disk 1.
 */

#ifndef TINYOS_FAT_H
#define TINYOS_FAT_H

#include "uefi/types.h"

/* Maximum path length */
#define FAT_MAX_PATH 512

/* Current working directory path (ASCII, null-terminated).
 * Lives in .bss — NOT on the stack. */
extern char g_cwd[FAT_MAX_PATH];

/* Initialize the filesystem layer.
 * Locates the Simple File System Protocol on the boot device,
 * opens the volume root, and sets it as the current directory.
 * Returns EFI_SUCCESS on success. */
EFI_STATUS fat_init();

/* List the contents of the directory at the given path.
 * If path is nullptr or empty, lists the current working directory.
 * Output goes to serial. */
EFI_STATUS fat_ls(const char *path);

/* Change the current working directory.
 * Supports relative paths, absolute paths (starting with /),
 * and ".." for parent directory.
 * Returns EFI_SUCCESS on success, EFI_NOT_FOUND if path doesn't exist. */
EFI_STATUS fat_cd(const char *path);

/* Create a new directory.
 * Returns EFI_SUCCESS on success. */
EFI_STATUS fat_mkdir(const char *name);

/* Utility: convert an ASCII string to a CHAR16 (UCS-2) wide string.
 * dst must have room for len+1 CHAR16 characters. */
void fat_ascii_to_wide(const char *src, CHAR16 *dst, int len);

/* Utility: convert a CHAR16 wide string to ASCII.
 * Non-ASCII characters are replaced with '?'. */
void fat_wide_to_ascii(const CHAR16 *src, char *dst, int max_len);

#endif /* TINYOS_FAT_H */
