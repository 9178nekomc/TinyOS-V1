/* TinyOS V1 — Main Entry Point (v2 — Graphical Boot + Installer)
 *
 * Boot flow:
 *   0. COM1 Serial Init (FIRST)
 *   1. Cache UEFI state
 *   2. GOP Init → show wallpaper for 3 seconds
 *   3. Switch to black graphical terminal
 *   4. Check system disk marker
 *      a. INSTALLED → "Starting TinyOS..." → Shell
 *      b. NOT INSTALLED → Installer wizard
 *   5. Shell REPL (on graphical terminal + serial)
 */

#include "uefi/types.h"
#include "serial.h"
#include "gop.h"
#include "ata.h"
#include "alloc.h"
#include "diskimg.h"
#include "fat.h"
#include "shell.h"
#include "gfxterm.h"
#include "sysdisk.h"
#include "install.h"
#include "pfs.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {

    /* ===================================================================
     * STEP 0: SERIAL FIRST — must be before any UEFI API call
     * =================================================================== */
    serial_init();
    serial_print("\r\n\r\n");
    serial_print("[MAIN] TinyOS V1 — Cold Boot\r\n");

    /* ===================================================================
     * STEP 1: Cache UEFI state
     * =================================================================== */
    g_ST = SystemTable;
    g_BS = SystemTable->BootServices;
    g_IH = ImageHandle;

    /* ===================================================================
     * STEP 2: GOP + Wallpaper (show for ~3 seconds)
     * =================================================================== */
    EFI_STATUS status = gop_init();
    if (!EFI_ERROR(status)) {
        /* Init ATA so we can read the wallpaper */
        ata_init();

        /* Load and render wallpaper */
        UINT8 sector0[512];
        status = ata_read(0, 1, sector0, ATA_DRIVE_SLAVE);
        if (!EFI_ERROR(status)) {
            /* Verify magic and render */
            bool magic_ok = true;
            const char *magic = "TINYOS_READY";
            for (int i = 0; i < 12; i++) {
                if (sector0[i] != (UINT8)magic[i]) magic_ok = false;
            }
            if (magic_ok) {
                serial_print("[MAIN] Loading wallpaper...\r\n");
                UINT8 *wp = (UINT8*)os_alloc(DISKIMG_WALLPAPER_SIZE);
                if (wp) {
                    UINT16 remaining = DISKIMG_WALLPAPER_SECTORS;
                    UINT64 lba = DISKIMG_WALLPAPER_LBA;
                    UINT8 *dst = wp;
                    while (remaining > 0) {
                        UINT16 n = (remaining > 128) ? 128 : remaining;
                        ata_read(lba, n, dst, ATA_DRIVE_SLAVE);
                        dst += n * 512; lba += n; remaining -= n;
                    }
                    gop_render_wallpaper(wp);
                    os_free(wp);
                    serial_print("[MAIN] Wallpaper displayed.\r\n");
                }
            }
        }

        /* Show the logo for ~3 seconds */
        for (volatile int i = 0; i < 60000000; i++) {
            __asm__ volatile("pause");
        }
    }

    /* ===================================================================
     * STEP 3: Switch to black graphical terminal
     * =================================================================== */
    serial_print("[MAIN] Starting graphical terminal...\r\n");
    if (gfxterm_init()) {
        gfxterm_clear();
    } else {
        serial_print("[MAIN] WARNING: GFX terminal init failed, using serial only\r\n");
    }

    /* ===================================================================
     * STEP 4: Initialize filesystem (ESP for shell commands)
     * =================================================================== */
    fat_init();

    /* ===================================================================
     * STEP 5: Check system disk → Install or Boot
     * =================================================================== */
    int sys_status = sysdisk_check();

    if (sys_status == 1) {
        /* ---- System installed → Boot directly ---- */
        serial_print("[MAIN] System disk found, booting...\r\n");
        gfxterm_print("\n\n");
        gfxterm_print("  TinyOS V1 — Starting from system disk...\n");
        gfxterm_print("  System marker verified.\n\n");
        /* Mount persistent file store */
        pfs_mount();
        gfxterm_print("  Persistent storage ready.\n");
        gfxterm_print("  Use pfls, mkfile, pfwrite, pfcat, pfrm.\n\n");
    } else {
        /* ---- No system → Install wizard ---- */
        serial_print("[MAIN] No system found, launching installer...\r\n");

        bool installed = install_run();

        if (installed) {
            /* User completed installation — restart the boot flow */
            serial_print("[MAIN] Installation complete. Returning to UEFI...\r\n");
            gfxterm_clear();
            gfxterm_print("\n\n  Rebooting...\n");
            for (volatile int i = 0; i < 30000000; i++) __asm__ volatile("pause");

            /* Return to UEFI — on next boot the system marker will be found */
            return EFI_SUCCESS;
        } else {
            /* User declined — enter live/shell mode */
            serial_print("[MAIN] Live mode (no system disk)\r\n");
        }
    }

    /* ===================================================================
     * STEP 6: Shell REPL
     * =================================================================== */
    gfxterm_print("  Type 'help' for available commands.\n\n");
    shell_init();
    shell_run();

    return EFI_SUCCESS;
}
