/* TinyOS V1 — Installation Wizard (ASCII-only) */

#include "install.h"
#include "gfxterm.h"
#include "sysdisk.h"
#include "ata.h"
#include "alloc.h"
#include "serial.h"

static void box_top() {
    gfxterm_print("  +------------------------------------------+\n");
}
static void box_mid() {
    gfxterm_print("  +------------------------------------------+\n");
}
static void box_bot() {
    gfxterm_print("  +------------------------------------------+\n");
}
static void box_line(const char *s) {
    gfxterm_print("  | "); gfxterm_print(s);
    int len = 0; while (s[len]) len++;
    for (int i = len; i < 40; i++) gfxterm_putc(' ');
    gfxterm_print(" |\n");
}
static void box_empty() { box_line(""); }

bool install_run() {
    char input[256];
    int choice = 0;

    /* ---- Welcome ---- */
    gfxterm_clear();
    gfxterm_print("\n\n");
    box_top();
    box_line("       TinyOS V1 -- System Installer");
    box_mid();
    box_empty();
    box_line("Welcome to TinyOS V1!");
    box_empty();
    box_line("No operating system was found on the");
    box_line("system disk. The installer will guide");
    box_line("you through setting up TinyOS on your");
    box_line("computer.");
    box_empty();
    box_line("WARNING: This will write data to the");
    box_line("selected disk. All existing data on");
    box_line("that disk will be lost.");
    box_empty();
    box_bot();
    gfxterm_print("\n  Install TinyOS V1 now? (yes/no): ");

    int len = gfxterm_readline(input, sizeof(input));
    /* Empty input (just Enter) = YES. Anything starting with 'n' or 'N' = no. */
    if (len > 0 && (input[0] == 'n' || input[0] == 'N')) {
        gfxterm_print("\n  Skipped. Live mode.\n\n");
        for (volatile int i = 0; i < 50000000; i++) __asm__ volatile("pause");
        return false;
    }

    /* ---- Disk Selection ---- */
    gfxterm_clear();
    gfxterm_print("\n\n");
    box_top();
    box_line("       Select Installation Disk");
    box_mid();
    box_empty();
    box_line("[1] Primary Slave  (hd0.img, 64MB)");
    box_line("[2] Secondary Master (hd1.img, 64MB)");
    box_empty();
    box_bot();
    gfxterm_print("\n  Enter disk number (1-2) [1]: ");

    len = gfxterm_readline(input, sizeof(input));
    if (len == 0 || input[0] == '\0') { choice = 1; }
    else if (input[0] >= '1' && input[0] <= '2') choice = input[0] - '0';
    else { gfxterm_print("\n  Invalid. Cancelled.\n"); return false; }

    bool is_secondary = (choice == 2);
    UINT8 drive_head = is_secondary ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE;
    gfxterm_print("\n  Selected: Disk ");
    gfxterm_print_dec(choice);
    gfxterm_print(is_secondary ? " (Secondary Master)\n" : " (Primary Slave)\n");

    /* ---- Installing (no confirmation needed) ---- */
    gfxterm_print("\n\n");
    box_top();
    box_line("       Installing TinyOS V1...");
    box_mid();
    gfxterm_print("\n");

    /* Helper macros for secondary vs primary */
    #define ATA_RD(lba, cnt, buf, dh) (is_secondary ? ata_read_sec(lba,cnt,buf,dh) : ata_read(lba,cnt,buf,dh))
    #define ATA_WR(lba, cnt, buf, dh) (is_secondary ? ata_write_sec(lba,cnt,buf,dh) : ata_write(lba,cnt,buf,dh))

    /* Step 1: Write system marker */
    gfxterm_print("  [1/4] Writing boot marker to LBA 1...");
    bool ok = is_secondary ? sysdisk_mark_installed_secondary() : sysdisk_mark_installed();
    gfxterm_print(ok ? " OK\n" : " FAILED!\n");
    if (!ok) { gfxterm_print("\n  Installation failed.\n"); return false; }

    /* Step 2: Format system area */
    gfxterm_print("  [2/4] Formatting system area...");
    {
        UINT8 *zero_sector = (UINT8*)os_alloc(512);
        if (zero_sector) {
            for (int i = 0; i < 512; i++) zero_sector[i] = 0;
            ATA_WR(2, 1, zero_sector, drive_head);
            for (int s = 3; s < 10; s++) ATA_WR(s, 1, zero_sector, drive_head);
            os_free(zero_sector);
            gfxterm_print(" OK\n");
        }
    }

    /* Step 3: Write system config */
    gfxterm_print("  [3/4] Writing system configuration...");
    {
        UINT8 *cfg = (UINT8*)os_alloc(512);
        if (cfg) {
            for (int i = 0; i < 512; i++) cfg[i] = 0;
            const char *info = "TINYOS V1 SYSTEM\r\nVERSION=1.0\r\nARCH=x86_64\r\nDATE=2026-07-04\r\n";
            for (int i = 0; info[i]; i++) cfg[i] = (UINT8)info[i];
            ATA_WR(2, 1, cfg, drive_head);
            os_free(cfg);
            gfxterm_print(" OK\n");
        }
    }

    /* Step 4: Verify */
    gfxterm_print("  [4/4] Verifying...");
    int check = sysdisk_check();
    gfxterm_print(check == 1 ? " OK\n" : " FAILED!\n");
    if (check != 1) { gfxterm_print("\n  Verification failed!\n"); return false; }

    #undef ATA_RD
    #undef ATA_WR

    /* ---- Done ---- */
    gfxterm_print("\n");
    box_top();
    box_line("     Installation Complete!");
    box_mid();
    box_empty();
    box_line("TinyOS V1 installed to:");
    gfxterm_print("  |   ");
    gfxterm_print(is_secondary ? "Secondary Master (hd1.img)\n" : "Primary Slave (hd0.img)\n");
    box_empty();
    box_line("The system will boot from this");
    box_line("disk on next startup.");
    box_empty();
    box_line("Press Enter to reboot...");
    box_bot();

    gfxterm_readline(input, sizeof(input));
    return true;
}
