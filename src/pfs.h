/* TinyOS V1 — Persistent File Store (PFS)
 *
 * A minimal sector-based file system on Disk 1 (raw disk0.img).
 * Uses LBA 2-999 (~498KB) for persistent file storage.
 *
 * Layout:
 *   LBA 0:    "TINYOS_READY" (disk validation)
 *   LBA 1:    "TINYOS_SYS" / "TINYOS_NOSYS" (system marker)
 *   LBA 2:    Superblock: "TINYOS_FS0" + free_bitmap[128] + metadata
 *   LBA 3:    Root directory: 16 entries x 32 bytes each
 *   LBA 4-999: Data blocks (996 blocks, ~498KB)
 *   LBA 1000+: Wallpaper (preserved)
 *
 * Each directory entry (32 bytes):
 *   name[24]  - filename (null-terminated, max 23 chars)
 *   size[4]   - file size in bytes (UINT32, little-endian)
 *   start[2]  - first data block (UINT16, little-endian, 0 = unused entry)
 *   flags[2]  - bit 0 = directory, bit 1 = read-only
 *
 * Each data block (first 2 bytes = next block pointer, rest = data):
 *   next[2]   - next block in chain (0xFFFF = last block)
 *   data[510] - file data
 */

#ifndef TINYOS_PFS_H
#define TINYOS_PFS_H

#include "uefi/types.h"

#define PFS_MAGIC        "TINYOS_FS0"
#define PFS_MAGIC_LEN    10
#define PFS_SUPER_LBA    2
#define PFS_DIR_LBA      3
#define PFS_DATA_START   4
#define PFS_DATA_END     999
#define PFS_TOTAL_BLOCKS  (PFS_DATA_END - PFS_DATA_START + 1)  /* 996 */
#define PFS_BLOCK_SIZE    512
#define PFS_DATA_SIZE     510   /* 512 - 2 bytes next pointer */
#define PFS_MAX_FILES     16
#define PFS_MAX_NAME      24

struct PfsDirEntry {
    char    name[24];
    UINT32  size;
    UINT16  start_block;
    UINT16  flags;
};

/* Initialize the PFS. Call once after detecting an installed system.
 * If the superblock has valid magic, mounts the existing filesystem.
 * Otherwise, formats a fresh one. */
bool pfs_mount();

/* Create a new file. Returns true on success.
 * Fails if the file already exists or the store is full. */
bool pfs_create(const char *name, bool is_dir);

/* List all files in the root directory. Output to gfxterm. */
void pfs_list();

/* Find a file by name. Returns entry index or -1 if not found. */
int pfs_find(const char *name);

/* Delete a file. Frees all its data blocks. */
bool pfs_delete(const char *name);

/* Get the number of free data blocks remaining. */
int pfs_free_blocks();

/* Write data to a file (creates or overwrites). */
bool pfs_write_file(const char *name, const UINT8 *data, UINT32 size);

/* Read file data into a pre-allocated buffer.
 * Buffer must be large enough. Returns actual bytes read. */
UINT32 pfs_read_file(const char *name, UINT8 *buf, UINT32 buf_size);

/* Check if PFS is mounted and ready. */
bool pfs_is_mounted();

#endif /* TINYOS_PFS_H */
