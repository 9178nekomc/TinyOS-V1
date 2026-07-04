/* TinyOS V1 — System Disk Detection
 *
 * While LBA 0 contains "TINYOS_READY" (disk validation),
 * LBA 1 contains "TINYOS_SYS" or "TINYOS_NOSYS" to indicate
 * whether the OS is fully installed on this disk.
 *
 * When the system is installed:
 *   - LBA 1:    b"TINYOS_SYS\0"  (12 bytes)
 *   - LBA 2:    System info / boot config (reserved)
 *   - LBA 3-999: System files area (~510 KB)
 *   - LBA 1000+: Wallpaper (4608 sectors)
 *
 * On first boot (no system installed):
 *   - LBA 1: b"TINYOS_NOSYS" or uninitialized
 *   - Shows install wizard
 */

#ifndef TINYOS_SYSDISK_H
#define TINYOS_SYSDISK_H

#include "uefi/types.h"

#define SYS_MARKER_LBA       1
#define SYS_MARKER_LEN       12
#define SYS_MARKER_INSTALLED "TINYOS_SYS"
#define SYS_MARKER_CLEAN     "TINYOS_NOSYS"

/* Check if system is installed on the slave disk.
 * Returns true if LBA 1 starts with "TINYOS_SYS". */
bool sysdisk_is_installed();

/* Write the "installed" marker to the slave disk.
 * Returns true on success. */
bool sysdisk_mark_installed();
bool sysdisk_mark_installed_secondary();

/* Write the "clean" marker (no system installed).
 * Called by the install wizard after a successful install. */
bool sysdisk_mark_clean();

/* Check the system marker and return a status code:
 *   0 = not checked
 *   1 = installed ("TINYOS_SYS")
 *   2 = not installed / empty
 *   3 = error reading disk
 */
int sysdisk_check();

#endif /* TINYOS_SYSDISK_H */
