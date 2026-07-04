/* TinyOS V1 — ATA PIO Driver
 *
 * Implements 28-bit LBA PIO read-only access to the Primary ATA bus
 * (PIIX3 PATA controller at I/O ports 0x1F0-0x1F7).
 *
 * CRITICAL RULE (from project constraints):
 *   Disk 1 is mounted as Primary Slave (index 1).
 *   The drive/head register (0x1F6) MUST be written with 0xF0,
 *   NOT 0xE0.  Writing 0xE0 targets the Primary Master (Disk 0 /
 *   ESP), which would read the wrong disk.
 *
 * We support LBA values up to 0x0FFFFFFF (28-bit), which covers
 * the entire 64 MB disk0.img.
 */

#ifndef TINYOS_ATA_H
#define TINYOS_ATA_H

#include "uefi/types.h"

/* PIIX3 Primary ATA Bus I/O ports */
#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERROR        0x1F1
#define ATA_PRIMARY_FEATURES     0x1F1
#define ATA_PRIMARY_SECTOR_COUNT 0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_STATUS       0x1F7
#define ATA_PRIMARY_COMMAND      0x1F7

/* Status register bits */
#define ATA_SR_BSY  0x80   /* Busy */
#define ATA_SR_DRDY 0x40   /* Device Ready */
#define ATA_SR_DF   0x20   /* Device Fault */
#define ATA_SR_DRQ  0x08   /* Data Request (ready to transfer) */
#define ATA_SR_ERR  0x01   /* Error */

/* ATA Commands */
#define ATA_CMD_READ_SECTORS    0x20   /* Read sectors (28-bit LBA, with retry) */
#define ATA_CMD_WRITE_SECTORS   0x30   /* Write sectors (28-bit LBA, with retry) */
#define ATA_CMD_IDENTIFY        0xEC   /* Identify device */

/* Drive/Head register values.
 * Bits 7-5 must be 101b for LBA mode.
 * Bit 4 = 0 for master, 1 for slave.
 * Bits 3-0 = LBA bits 27-24. */
#define ATA_DRIVE_MASTER  0xE0   /* 1110 0000 = LBA mode + master */
#define ATA_DRIVE_SLAVE   0xF0   /* 1111 0000 = LBA mode + slave  ← USE THIS */

/* Secondary ATA bus (0x170) */
#define ATA_SECONDARY_IO   0x170
#define ATA_SECONDARY_CTRL 0x376

/* Initialize the ATA controller. Resets both devices and waits
 * for them to become ready. Called once at boot. */
EFI_STATUS ata_init();

/* Read `count` sectors starting at `lba` from the specified device
 * into `buf`. buf must be at least count * 512 bytes.
 *
 * drive_head: ATA_DRIVE_MASTER (0xE0) for primary master,
 *             ATA_DRIVE_SLAVE  (0xF0) for primary slave.
 *
 * Returns EFI_SUCCESS on success, EFI_DEVICE_ERROR on failure. */
EFI_STATUS ata_read(UINT64 lba, UINT16 count, UINT8 *buf, UINT8 drive_head);

/* Write `count` sectors from `buf` to the specified device.
 * Same parameters as ata_read.
 * WARNING: This modifies the disk. Use with care. */
EFI_STATUS ata_write(UINT64 lba, UINT16 count, const UINT8 *buf, UINT8 drive_head);

/* Secondary bus (0x170) — for disks index >= 2 */
EFI_STATUS ata_read_sec(UINT64 lba, UINT16 count, UINT8 *buf, UINT8 drive_head);
EFI_STATUS ata_write_sec(UINT64 lba, UINT16 count, const UINT8 *buf, UINT8 drive_head);

#endif /* TINYOS_ATA_H */
