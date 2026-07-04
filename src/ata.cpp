/* TinyOS V1 — ATA PIO Driver Implementation
 *
 * 28-bit LBA PIO mode read-only driver for the PIIX3 PATA controller.
 * Uses the Primary bus at I/O ports 0x1F0-0x1F7.
 */

#include "ata.h"
#include "serial.h"

/* ===========================================================================
 * Low-level I/O port helpers (same pattern as serial.cpp)
 * =========================================================================== */

static inline void outb(UINT16 port, UINT8 value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline UINT8 inb(UINT16 port) {
    UINT8 value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline UINT16 inw(UINT16 port) {
    UINT16 value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(UINT16 port, UINT16 value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/* ===========================================================================
 * Internal helpers
 * =========================================================================== */

/* Wait for BSY to clear (device is ready to accept commands).
 * Returns true if device became ready, false on timeout. */
static bool ata_wait_not_busy() {
    /* Timeout: ~100ms at ~1us per iteration */
    for (int timeout = 0; timeout < 100000; timeout++) {
        UINT8 status = inb(ATA_PRIMARY_STATUS);
        if (!(status & ATA_SR_BSY)) {
            return true;
        }
        /* Small delay */
        __asm__ volatile("pause");
    }
    return false;
}

/* Wait for DRQ to be set (device has data ready to transfer).
 * Returns true if data is ready, false on timeout or error. */
static bool ata_wait_drq() {
    for (int timeout = 0; timeout < 100000; timeout++) {
        UINT8 status = inb(ATA_PRIMARY_STATUS);
        if (status & ATA_SR_ERR) {
            serial_print("[ATA] ERROR: Device reported error status\r\n");
            return false;
        }
        if (status & ATA_SR_DRQ) {
            return true;
        }
        __asm__ volatile("pause");
    }
    return false;
}

/* 400ns delay after each sector.
 * Reading the status register on PIIX3 takes ~100ns, so 4 reads
 * provide the required 400ns minimum delay. */
static void ata_400ns_delay() {
    for (int i = 0; i < 4; i++) {
        inb(ATA_PRIMARY_STATUS);
    }
}

/* ===========================================================================
 * Public API
 * =========================================================================== */

EFI_STATUS ata_init() {
    serial_print("[ATA] Initializing ATA controller...\r\n");

    /* Issue a software reset via the Device Control register (0x3F6).
     * Write bit 2 (SRST) then clear it. */
    outb(0x3F6, 0x04);  /* Set SRST */
    /* Wait at least 5us — reading status gives ~100ns per read */
    for (int i = 0; i < 100; i++) inb(ATA_PRIMARY_STATUS);
    outb(0x3F6, 0x00);  /* Clear SRST */
    for (int i = 0; i < 100; i++) inb(ATA_PRIMARY_STATUS);

    /* Select slave device and wait for it to become ready */
    outb(ATA_PRIMARY_DRIVE_HEAD, ATA_DRIVE_SLAVE);

    if (!ata_wait_not_busy()) {
        serial_print("[ATA] WARNING: Slave device did not become ready\r\n");
        /* Continue anyway — it might work when we actually read */
    }

    /* Check if device is present by reading status */
    UINT8 status = inb(ATA_PRIMARY_STATUS);
    serial_print("[ATA] Slave status: ");
    serial_print_hex(status);
    if (status == 0xFF) {
        serial_print(" (no device?)\r\n");
    } else {
        serial_print(" (device detected)\r\n");
    }

    serial_print("[ATA] Controller initialized.\r\n");
    return EFI_SUCCESS;
}

EFI_STATUS ata_read(UINT64 lba, UINT16 count, UINT8 *buf, UINT8 drive_head) {
    if (buf == nullptr || count == 0) {
        return EFI_INVALID_PARAMETER;
    }

    /* Check LBA is within 28-bit range */
    if (lba > 0x0FFFFFFF) {
        serial_print("[ATA] ERROR: LBA exceeds 28-bit range: ");
        serial_print_hex(lba);
        serial_print("\r\n");
        return EFI_INVALID_PARAMETER;
    }

    /* Wait for controller to be ready */
    if (!ata_wait_not_busy()) {
        serial_print("[ATA] ERROR: Controller busy timeout before command\r\n");
        return EFI_DEVICE_ERROR;
    }

    /* Select device and set LBA[27:24] in the top 4 bits of drive/head.
     * drive_head already has the master/slave bit set.
     *   Bits 7-5: 101 (LBA mode)
     *   Bit 4:    0=master, 1=slave (already in drive_head)
     *   Bits 3-0: LBA[27:24] */
    UINT8 dh = drive_head | ((lba >> 24) & 0x0F);
    outb(ATA_PRIMARY_DRIVE_HEAD, dh);

    /* Set sector count */
    outb(ATA_PRIMARY_SECTOR_COUNT, (UINT8)(count & 0xFF));

    /* Set LBA[7:0] */
    outb(ATA_PRIMARY_LBA_LO, (UINT8)(lba & 0xFF));

    /* Set LBA[15:8] */
    outb(ATA_PRIMARY_LBA_MID, (UINT8)((lba >> 8) & 0xFF));

    /* Set LBA[23:16] */
    outb(ATA_PRIMARY_LBA_HI, (UINT8)((lba >> 16) & 0xFF));

    /* Issue READ SECTORS command */
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_READ_SECTORS);

    /* Read each sector */
    for (UINT16 s = 0; s < count; s++) {
        /* Wait for data to be ready */
        if (!ata_wait_drq()) {
            serial_print("[ATA] ERROR: DRQ timeout on sector ");
            serial_print_dec(s);
            serial_print(" of ");
            serial_print_dec(count);
            serial_print("\r\n");
            return EFI_DEVICE_ERROR;
        }

        /* Read 256 words (512 bytes) from the data port.
         * We use rep insw for efficiency. */
        UINT16 *buf16 = (UINT16*)buf;
        for (int i = 0; i < 256; i++) {
            UINT16 word = inw(ATA_PRIMARY_DATA);
            /* ATA returns data in little-endian word order.
             * Each word's bytes are already in the correct order
             * for x86 (little-endian), so just store as-is. */
            buf16[i] = word;
        }

        buf += 512;  /* Advance to next sector buffer */

        /* 400ns minimum delay between sectors */
        ata_400ns_delay();
    }

    return EFI_SUCCESS;
}

/* ===========================================================================
 * ATA Write — PIO28 WRITE SECTORS (0x30)
 * =========================================================================== */

EFI_STATUS ata_write(UINT64 lba, UINT16 count, const UINT8 *buf, UINT8 drive_head) {
    if (buf == nullptr || count == 0) {
        return EFI_INVALID_PARAMETER;
    }

    if (lba > 0x0FFFFFFF) {
        serial_print("[ATA] ERROR: Write LBA out of range\r\n");
        return EFI_INVALID_PARAMETER;
    }

    if (!ata_wait_not_busy()) {
        serial_print("[ATA] ERROR: Busy before write\r\n");
        return EFI_DEVICE_ERROR;
    }

    UINT8 dh = drive_head | ((lba >> 24) & 0x0F);
    outb(ATA_PRIMARY_DRIVE_HEAD, dh);
    outb(ATA_PRIMARY_SECTOR_COUNT, (UINT8)(count & 0xFF));
    outb(ATA_PRIMARY_LBA_LO, (UINT8)(lba & 0xFF));
    outb(ATA_PRIMARY_LBA_MID, (UINT8)((lba >> 8) & 0xFF));
    outb(ATA_PRIMARY_LBA_HI, (UINT8)((lba >> 16) & 0xFF));
    outb(ATA_PRIMARY_COMMAND, ATA_CMD_WRITE_SECTORS);

    for (UINT16 s = 0; s < count; s++) {
        for (int timeout = 0; timeout < 100000; timeout++) {
            UINT8 status = inb(ATA_PRIMARY_STATUS);
            if (status & ATA_SR_ERR) {
                serial_print("[ATA] Write error on sector ");
                serial_print_dec(s);
                serial_print("\r\n");
                return EFI_DEVICE_ERROR;
            }
            if (status & ATA_SR_DRQ) break;
            __asm__ volatile("pause");
        }

        const UINT16 *buf16 = (const UINT16*)buf;
        for (int i = 0; i < 256; i++) {
            outw(ATA_PRIMARY_DATA, buf16[i]);
        }
        buf += 512;
        ata_400ns_delay();
    }

    ata_wait_not_busy();
    return EFI_SUCCESS;
}

/* ===========================================================================
 * Generic PIO read/write (parameterized by I/O base)
 * =========================================================================== */

static EFI_STATUS ata_pio_read(UINT64 lba, UINT16 count, UINT8 *buf, UINT8 drive_head,
                                UINT16 io_base, UINT16 ctrl_base) {
    if (buf == nullptr || count == 0) return EFI_INVALID_PARAMETER;
    if (lba > 0x0FFFFFFF) return EFI_INVALID_PARAMETER;

    if (!ata_wait_not_busy()) return EFI_DEVICE_ERROR;

    UINT16 DATA=io_base, ERR=io_base+1, SECCOUNT=io_base+2,
           LBA_LO=io_base+3, LBA_MID=io_base+4, LBA_HI=io_base+5,
           DRIVE=io_base+6, STATUS=io_base+7, CMD=io_base+7;

    UINT8 dh = drive_head | ((lba >> 24) & 0x0F);
    outb(DRIVE, dh);
    outb(SECCOUNT, (UINT8)(count & 0xFF));
    outb(LBA_LO, (UINT8)(lba & 0xFF));
    outb(LBA_MID, (UINT8)((lba >> 8) & 0xFF));
    outb(LBA_HI, (UINT8)((lba >> 16) & 0xFF));
    outb(CMD, ATA_CMD_READ_SECTORS);

    for (UINT16 s = 0; s < count; s++) {
        for (int timeout = 0; timeout < 100000; timeout++) {
            UINT8 st = inb(STATUS);
            if (st & ATA_SR_ERR) return EFI_DEVICE_ERROR;
            if (st & ATA_SR_DRQ) break;
            __asm__ volatile("pause");
        }
        UINT16 *buf16 = (UINT16*)buf;
        for (int i = 0; i < 256; i++) buf16[i] = inw(DATA);
        buf += 512;
        for (int i = 0; i < 4; i++) inb(STATUS);
    }
    return EFI_SUCCESS;
}

static EFI_STATUS ata_pio_write(UINT64 lba, UINT16 count, const UINT8 *buf, UINT8 drive_head,
                                 UINT16 io_base, UINT16 ctrl_base) {
    if (buf == nullptr || count == 0) return EFI_INVALID_PARAMETER;
    if (lba > 0x0FFFFFFF) return EFI_INVALID_PARAMETER;
    if (!ata_wait_not_busy()) return EFI_DEVICE_ERROR;

    UINT16 DATA=io_base, SECCOUNT=io_base+2,
           LBA_LO=io_base+3, LBA_MID=io_base+4, LBA_HI=io_base+5,
           DRIVE=io_base+6, CMD=io_base+7;

    UINT8 dh = drive_head | ((lba >> 24) & 0x0F);
    outb(DRIVE, dh);
    outb(SECCOUNT, (UINT8)(count & 0xFF));
    outb(LBA_LO, (UINT8)(lba & 0xFF));
    outb(LBA_MID, (UINT8)((lba >> 8) & 0xFF));
    outb(LBA_HI, (UINT8)((lba >> 16) & 0xFF));
    outb(CMD, ATA_CMD_WRITE_SECTORS);

    for (UINT16 s = 0; s < count; s++) {
        for (int timeout = 0; timeout < 100000; timeout++) {
            UINT8 st = inb(io_base+7);
            if (st & ATA_SR_ERR) return EFI_DEVICE_ERROR;
            if (st & ATA_SR_DRQ) break;
            __asm__ volatile("pause");
        }
        const UINT16 *buf16 = (const UINT16*)buf;
        for (int i = 0; i < 256; i++) outw(DATA, buf16[i]);
        buf += 512;
        for (int i = 0; i < 4; i++) inb(io_base+7);
    }
    ata_wait_not_busy();
    return EFI_SUCCESS;
}

/* Secondary bus wrappers */
EFI_STATUS ata_read_sec(UINT64 lba, UINT16 count, UINT8 *buf, UINT8 drive_head) {
    return ata_pio_read(lba, count, buf, drive_head, 0x170, 0x376);
}
EFI_STATUS ata_write_sec(UINT64 lba, UINT16 count, const UINT8 *buf, UINT8 drive_head) {
    return ata_pio_write(lba, count, buf, drive_head, 0x170, 0x376);
}
