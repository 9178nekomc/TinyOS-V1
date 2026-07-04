/* TinyOS V1 — Persistent File Store Implementation */

#include "pfs.h"
#include "ata.h"
#include "alloc.h"
#include "serial.h"
#include "gfxterm.h"

static bool g_pfs_mounted = false;
static UINT8 g_bitmap[128]; /* 128 bytes = 1024 bits (covers 996 blocks comfortably) */

/* ===========================================================================
 * Internal helpers
 * =========================================================================== */

static void mem_set(void *p, UINT8 v, int n) {
    UINT8 *b = (UINT8*)p; for (int i = 0; i < n; i++) b[i] = v;
}

static void mem_cpy(void *d, const void *s, int n) {
    UINT8 *dd = (UINT8*)d; const UINT8 *ss = (const UINT8*)s;
    for (int i = 0; i < n; i++) dd[i] = ss[i];
}

static bool str_eq_n(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) { if (a[i] != b[i]) return false; if (a[i] == 0) return true; }
    return true;
}

static void write_u16(UINT8 *p, UINT16 v) { p[0]=v&0xFF; p[1]=(v>>8)&0xFF; }
static void write_u32(UINT8 *p, UINT32 v) { p[0]=v&0xFF;p[1]=(v>>8)&0xFF;p[2]=(v>>16)&0xFF;p[3]=(v>>24)&0xFF; }
static UINT16 read_u16(const UINT8 *p) { return p[0]|(p[1]<<8); }
static UINT32 read_u32(const UINT8 *p) { return p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24); }

/* ===========================================================================
 * Block allocator
 * =========================================================================== */

static void bitmap_rebuild() {
    /* Read all data blocks to rebuild the free bitmap */
    mem_set(g_bitmap, 0, sizeof(g_bitmap));

    /* Mark blocks 0..(PFS_TOTAL_BLOCKS-1) as initially free */
    for (int i = 0; i < PFS_TOTAL_BLOCKS; i++) {
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        g_bitmap[byte_idx] &= ~(1 << bit_idx); /* 0 = free */
    }

    /* Read root directory to mark allocated blocks */
    UINT8 dir_sector[512];
    EFI_STATUS st = ata_read(PFS_DIR_LBA, 1, dir_sector, ATA_DRIVE_SLAVE);
    if (EFI_ERROR(st)) return;

    for (int e = 0; e < PFS_MAX_FILES; e++) {
        UINT8 *entry = dir_sector + e * 32;
        UINT16 start = read_u16(entry + 28);
        if (start == 0 || start < PFS_DATA_START || start > PFS_DATA_END) continue;

        /* Follow the chain */
        UINT16 block = start;
        while (block >= PFS_DATA_START && block <= PFS_DATA_END) {
            int idx = block - PFS_DATA_START;
            int byte_idx = idx / 8;
            int bit_idx = idx % 8;
            g_bitmap[byte_idx] |= (1 << bit_idx); /* 1 = allocated */

            UINT8 blk[512];
            if (EFI_ERROR(ata_read(block, 1, blk, ATA_DRIVE_SLAVE))) break;
            UINT16 next = read_u16(blk);
            if (next == 0xFFFF) break;
            block = next;
        }
    }
}

static int bitmap_alloc() {
    for (int i = 0; i < PFS_TOTAL_BLOCKS; i++) {
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        if (!(g_bitmap[byte_idx] & (1 << bit_idx))) {
            g_bitmap[byte_idx] |= (1 << bit_idx);
            return PFS_DATA_START + i;
        }
    }
    return -1; /* Full */
}

static void bitmap_free(int block) {
    int idx = block - PFS_DATA_START;
    if (idx < 0 || idx >= PFS_TOTAL_BLOCKS) return;
    g_bitmap[idx / 8] &= ~(1 << (idx % 8));
}

/* ===========================================================================
 * Mount / Format
 * =========================================================================== */

bool pfs_mount() {
    serial_print("[PFS] Mounting persistent file store...\r\n");

    /* Read superblock */
    UINT8 super[512];
    EFI_STATUS st = ata_read(PFS_SUPER_LBA, 1, super, ATA_DRIVE_SLAVE);
    if (EFI_ERROR(st)) {
        serial_print("[PFS] Cannot read superblock\r\n");
        goto format;
    }

    /* Check magic */
    const char *magic = PFS_MAGIC;
    bool valid = true;
    for (int i = 0; i < PFS_MAGIC_LEN; i++) {
        if (super[i] != (UINT8)magic[i]) valid = false;
    }

    if (!valid) {
        serial_print("[PFS] No valid filesystem found, formatting...\r\n");
        goto format;
    }

    /* Mounted successfully */
    serial_print("[PFS] Filesystem mounted.\r\n");
    bitmap_rebuild();
    g_pfs_mounted = true;
    return true;

format:
    /* Format a new filesystem */
    mem_set(super, 0, 512);
    for (int i = 0; magic[i]; i++) super[i] = (UINT8)magic[i];
    super[16] = 0x01; /* version */
    ata_write(PFS_SUPER_LBA, 1, super, ATA_DRIVE_SLAVE);

    /* Clear root directory */
    UINT8 dir[512];
    mem_set(dir, 0, 512);
    ata_write(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE);

    /* Rebuild bitmap (all free) */
    mem_set(g_bitmap, 0, sizeof(g_bitmap));

    serial_print("[PFS] Fresh filesystem created.\r\n");
    g_pfs_mounted = true;
    return true;
}

bool pfs_is_mounted() { return g_pfs_mounted; }

int pfs_free_blocks() {
    int n = 0;
    for (int i = 0; i < PFS_TOTAL_BLOCKS; i++) {
        int byte_idx = i / 8, bit_idx = i % 8;
        if (!(g_bitmap[byte_idx] & (1 << bit_idx))) n++;
    }
    return n;
}

/* ===========================================================================
 * File operations
 * =========================================================================== */

int pfs_find(const char *name) {
    UINT8 dir[512];
    if (EFI_ERROR(ata_read(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE))) return -1;

    for (int e = 0; e < PFS_MAX_FILES; e++) {
        UINT8 *entry = dir + e * 32;
        if (entry[0] == 0) continue; /* Empty */
        if (str_eq_n((const char*)entry, name, PFS_MAX_NAME)) return e;
    }
    return -1;
}

bool pfs_create(const char *name, bool is_dir) {
    if (!g_pfs_mounted) return false;
    if (pfs_find(name) >= 0) return false; /* Already exists */

    UINT8 dir[512];
    if (EFI_ERROR(ata_read(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE))) return false;

    /* Find free entry */
    int free_entry = -1;
    for (int e = 0; e < PFS_MAX_FILES; e++) {
        if (dir[e * 32] == 0) { free_entry = e; break; }
    }
    if (free_entry < 0) return false;

    /* Write entry */
    UINT8 *entry = dir + free_entry * 32;
    mem_set(entry, 0, 32);
    int name_len = 0; while (name[name_len] && name_len < 23) name_len++;
    mem_cpy(entry, name, name_len);
    write_u32(entry + 24, 0); /* size = 0 initially */
    write_u16(entry + 28, 0); /* no data blocks yet */
    write_u16(entry + 30, is_dir ? 1 : 0);

    if (EFI_ERROR(ata_write(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE))) return false;
    serial_print("[PFS] Created: "); serial_print(name); serial_print("\r\n");
    return true;
}

void pfs_list() {
    UINT8 dir[512];
    if (EFI_ERROR(ata_read(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE))) {
        gfxterm_print("  Error reading directory.\n");
        return;
    }

    int count = 0;
    gfxterm_print("\n");
    for (int e = 0; e < PFS_MAX_FILES; e++) {
        UINT8 *entry = dir + e * 32;
        if (entry[0] == 0) continue;
        count++;
        UINT32 size = read_u32(entry + 24);
        UINT16 flags = read_u16(entry + 30);
        bool is_dir = (flags & 1) != 0;

        gfxterm_print(is_dir ? "  [DIR]  " : "         ");
        gfxterm_print((const char*)entry);
        if (!is_dir) {
            gfxterm_print("  (");
            gfxterm_print_dec(size);
            gfxterm_print(" bytes)");
        }
        gfxterm_print("\n");
    }
    if (count == 0) gfxterm_print("  (empty)\n");
    gfxterm_print("  Free: "); gfxterm_print_dec(pfs_free_blocks());
    gfxterm_print(" blocks (");
    gfxterm_print_dec(pfs_free_blocks() * PFS_DATA_SIZE / 1024);
    gfxterm_print(" KB)\n\n");
}

bool pfs_delete(const char *name) {
    int idx = pfs_find(name);
    if (idx < 0) return false;

    UINT8 dir[512];
    if (EFI_ERROR(ata_read(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE))) return false;

    UINT8 *entry = dir + idx * 32;
    UINT16 block = read_u16(entry + 28);

    /* Free the block chain */
    while (block >= PFS_DATA_START && block <= PFS_DATA_END) {
        UINT8 blk[512];
        if (EFI_ERROR(ata_read(block, 1, blk, ATA_DRIVE_SLAVE))) break;
        UINT16 next = read_u16(blk);
        bitmap_free(block);
        if (next == 0xFFFF) break;
        block = next;
    }

    /* Clear directory entry */
    mem_set(entry, 0, 32);
    if (EFI_ERROR(ata_write(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE))) return false;
    return true;
}

bool pfs_write_file(const char *name, const UINT8 *data, UINT32 size) {
    if (!g_pfs_mounted) return false;

    /* Delete existing file first */
    pfs_delete(name);

    /* Create new entry */
    if (!pfs_create(name, false)) return false;

    int idx = pfs_find(name);
    if (idx < 0) return false;

    /* Allocate blocks and write data */
    UINT32 bytes_left = size;
    UINT16 prev_block = 0;
    UINT16 first_block = 0;

    while (bytes_left > 0) {
        int block = bitmap_alloc();
        if (block < 0) { /* Out of space — clean up */ pfs_delete(name); return false; }

        if (first_block == 0) first_block = block;

        UINT8 blk[512];
        mem_set(blk, 0, 512);

        UINT32 chunk = (bytes_left > PFS_DATA_SIZE) ? PFS_DATA_SIZE : bytes_left;
        mem_cpy(blk + 2, data, chunk);
        data += chunk;
        bytes_left -= chunk;

        /* Set next pointer */
        if (bytes_left > 0) {
            /* Will allocate another block — leave next=0 for now, update later */
            write_u16(blk, 0xFFFF); /* Temporary — will be updated if we allocate more */
        } else {
            write_u16(blk, 0xFFFF); /* Last block */
        }

        if (EFI_ERROR(ata_write(block, 1, blk, ATA_DRIVE_SLAVE))) {
            pfs_delete(name); return false;
        }

        /* Update previous block's next pointer */
        if (prev_block != 0) {
            UINT8 prev_blk[512];
            ata_read(prev_block, 1, prev_blk, ATA_DRIVE_SLAVE);
            write_u16(prev_blk, block);
            ata_write(prev_block, 1, prev_blk, ATA_DRIVE_SLAVE);
        }
        prev_block = block;
    }

    /* Update directory entry with size and first block */
    UINT8 dir[512];
    ata_read(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE);
    UINT8 *entry = dir + idx * 32;
    write_u32(entry + 24, size);
    write_u16(entry + 28, first_block);
    ata_write(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE);

    serial_print("[PFS] Wrote "); serial_print_dec(size);
    serial_print(" bytes to '"); serial_print(name); serial_print("'\r\n");
    return true;
}

UINT32 pfs_read_file(const char *name, UINT8 *buf, UINT32 buf_size) {
    int idx = pfs_find(name);
    if (idx < 0) return 0;

    UINT8 dir[512];
    if (EFI_ERROR(ata_read(PFS_DIR_LBA, 1, dir, ATA_DRIVE_SLAVE))) return 0;

    UINT8 *entry = dir + idx * 32;
    UINT32 size = read_u32(entry + 24);
    UINT16 block = read_u16(entry + 28);

    if (size > buf_size) size = buf_size;
    UINT32 read = 0;

    while (block >= PFS_DATA_START && block <= PFS_DATA_END && read < size) {
        UINT8 blk[512];
        if (EFI_ERROR(ata_read(block, 1, blk, ATA_DRIVE_SLAVE))) break;

        UINT32 chunk = size - read;
        if (chunk > PFS_DATA_SIZE) chunk = PFS_DATA_SIZE;
        mem_cpy(buf + read, blk + 2, chunk);
        read += chunk;

        UINT16 next = read_u16(blk);
        if (next == 0xFFFF) break;
        block = next;
    }

    return read;
}
