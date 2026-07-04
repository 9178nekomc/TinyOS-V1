/* TinyOS V1 — Disk Image Parser Implementation */

#include "diskimg.h"
#include "ata.h"
#include "alloc.h"
#include "gop.h"
#include "serial.h"

/* ===========================================================================
 * Internal helpers
 * =========================================================================== */

/* Compare n bytes of memory. Returns true if equal. */
static bool mem_eq(const UINT8 *a, const char *b, UINTN n) {
    for (UINTN i = 0; i < n; i++) {
        if (a[i] != (UINT8)b[i]) return false;
    }
    return true;
}

/* ===========================================================================
 * Public API
 * =========================================================================== */

bool diskimg_verify_and_load_wallpaper() {
    UINT8 *buf = nullptr;

    /* Step 1: Read sector 0 (LBA 0) to verify magic signature.
     * We use a small .bss-based buffer here since it's only 512 bytes.
     * Actually, we stack-allocate this one tiny buffer (512 bytes is safe). */
    UINT8 sector0[512];
    for (int i = 0; i < 512; i++) sector0[i] = 0;

    serial_print("[DISKIMG] Reading LBA 0 for magic verification...\r\n");

    EFI_STATUS status = ata_read(0, 1, sector0, ATA_DRIVE_SLAVE);
    if (EFI_ERROR(status)) {
        serial_print("[DISKIMG] FATAL: Cannot read LBA 0 from slave device.\r\n");
        serial_print("[DISKIMG] Is disk0.img attached as Primary Slave (index 1)?\r\n");
        serial_print("[DISKIMG] System halted.\r\n");
        for (;;) { __asm__ volatile("hlt"); }
        return false;
    }

    /* Verify magic */
    if (!mem_eq(sector0, DISKIMG_MAGIC, DISKIMG_MAGIC_LEN)) {
        serial_print("[DISKIMG] FATAL: Magic signature mismatch!\r\n");
        serial_print("[DISKIMG] Expected: " DISKIMG_MAGIC "\r\n");
        serial_print("[DISKIMG] Got:       ");
        for (int i = 0; i < DISKIMG_MAGIC_LEN; i++) {
            serial_putc(sector0[i] >= 0x20 && sector0[i] <= 0x7E ? sector0[i] : '?');
        }
        serial_print("\r\n");
        serial_print("[DISKIMG] Wrong disk or corrupted image. System halted.\r\n");
        for (;;) { __asm__ volatile("hlt"); }
        return false;
    }

    serial_print("[DISKIMG] Magic signature verified: " DISKIMG_MAGIC "\r\n");

    /* Step 2: Allocate wallpaper buffer via UEFI pool.
     * CRITICAL: 2.25MB — must NOT be on the stack! */
    serial_print("[DISKIMG] Allocating wallpaper buffer (");
    serial_print_dec(DISKIMG_WALLPAPER_SIZE);
    serial_print(" bytes)...\r\n");

    buf = (UINT8*)os_alloc(DISKIMG_WALLPAPER_SIZE);
    serial_print("[DISKIMG] Buffer allocated at ");
    serial_print_hex((UINT64)(UINTN)buf);
    serial_print("\r\n");

    /* Step 3: Read wallpaper from disk in chunks.
     * 128 sectors per read = 64KB per chunk (fits in ATA sector count byte).
     * Total: 4608 sectors, 36 chunks. */
    serial_print("[DISKIMG] Reading wallpaper from LBA ");
    serial_print_dec(DISKIMG_WALLPAPER_LBA);
    serial_print(" (");
    serial_print_dec(DISKIMG_WALLPAPER_SECTORS);
    serial_print(" sectors)...\r\n");

    #define CHUNK_SECTORS 128
    UINT8 *dst = buf;
    UINT16 remaining = DISKIMG_WALLPAPER_SECTORS;
    UINT64 current_lba = DISKIMG_WALLPAPER_LBA;

    while (remaining > 0) {
        UINT16 count = (remaining > CHUNK_SECTORS) ? CHUNK_SECTORS : remaining;

        status = ata_read(current_lba, count, dst, ATA_DRIVE_SLAVE);
        if (EFI_ERROR(status)) {
            serial_print("[DISKIMG] ERROR: ATA read failed at LBA ");
            serial_print_dec(current_lba);
            serial_print(", count=");
            serial_print_dec(count);
            serial_print("\r\n");
            os_free(buf);
            serial_print("[DISKIMG] System halted.\r\n");
            for (;;) { __asm__ volatile("hlt"); }
            return false;
        }

        dst         += count * 512;
        current_lba += count;
        remaining   -= count;

        /* Progress indicator */
        serial_putc('.');
    }
    serial_print(" done.\r\n");

    /* Step 4: Render the wallpaper to the GOP framebuffer */
    gop_render_wallpaper(buf);

    /* Step 5: Free the wallpaper buffer — no longer needed */
    os_free(buf);
    serial_print("[DISKIMG] Wallpaper buffer freed.\r\n");

    return true;
}
