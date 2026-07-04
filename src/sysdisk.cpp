/* TinyOS V1 — System Disk Detection (dual-disk) */

#include "sysdisk.h"
#include "ata.h"
#include "serial.h"
#include "alloc.h"

static bool mem_cmp(const UINT8 *a, const char *b, int n) {
    for (int i = 0; i < n; i++) if (a[i] != (UINT8)b[i]) return false;
    return true;
}

int sysdisk_check() {
    UINT8 sector[512];

    /* Check Primary Slave (index=1, hd0.img) */
    EFI_STATUS st = ata_read(SYS_MARKER_LBA, 1, sector, ATA_DRIVE_SLAVE);
    if (!EFI_ERROR(st) && mem_cmp(sector, SYS_MARKER_INSTALLED, SYS_MARKER_LEN)) {
        serial_print("[SYSDISK] INSTALLED on Primary Slave\r\n");
        return 1;
    }

    /* Check Secondary Master (index=2, hd1.img) */
    st = ata_read_sec(SYS_MARKER_LBA, 1, sector, ATA_DRIVE_MASTER);
    if (!EFI_ERROR(st) && mem_cmp(sector, SYS_MARKER_INSTALLED, SYS_MARKER_LEN)) {
        serial_print("[SYSDISK] INSTALLED on Secondary Master\r\n");
        return 1;
    }

    serial_print("[SYSDISK] NOT INSTALLED\r\n");
    return 2;
}

bool sysdisk_is_installed() { return sysdisk_check() == 1; }

static bool write_marker_to(UINT64 lba, const char *marker,
                             UINT8 drive_head, bool is_secondary) {
    UINT8 *sector = (UINT8*)os_alloc(512);
    if (!sector) return false;
    EFI_STATUS st;
    if (is_secondary)
        st = ata_read_sec(lba, 1, sector, drive_head);
    else
        st = ata_read(lba, 1, sector, drive_head);
    if (EFI_ERROR(st)) { os_free(sector); return false; }
    for (int i = 0; i < SYS_MARKER_LEN; i++) sector[i] = (UINT8)marker[i];
    if (is_secondary)
        st = ata_write_sec(lba, 1, sector, drive_head);
    else
        st = ata_write(lba, 1, sector, drive_head);
    os_free(sector);
    return !EFI_ERROR(st);
}

bool sysdisk_mark_installed() {
    serial_print("[SYSDISK] Writing INSTALLED to Primary Slave...\r\n");
    return write_marker_to(SYS_MARKER_LBA, SYS_MARKER_INSTALLED, ATA_DRIVE_SLAVE, false);
}

bool sysdisk_mark_installed_secondary() {
    serial_print("[SYSDISK] Writing INSTALLED to Secondary Master...\r\n");
    return write_marker_to(SYS_MARKER_LBA, SYS_MARKER_INSTALLED, ATA_DRIVE_MASTER, true);
}

bool sysdisk_mark_clean() {
    serial_print("[SYSDISK] Writing CLEAN marker...\r\n");
    write_marker_to(SYS_MARKER_LBA, SYS_MARKER_CLEAN, ATA_DRIVE_SLAVE, false);
    write_marker_to(SYS_MARKER_LBA, SYS_MARKER_CLEAN, ATA_DRIVE_MASTER, true);
    return true;
}
