/* TinyOS V1 — Disk Image Parser
 *
 * Validates the disk0.img format and loads wallpaper data.
 *
 * disk0.img layout:
 *   LBA 0:      12-byte magic signature "TINYOS_READY" (ASCII)
 *   LBA 1000:   1024×768 BGR24 pixel matrix (2,359,296 bytes, 4608 sectors)
 *
 * The magic signature at LBA 0 serves as a safety check to ensure
 * we're reading from the correct disk (Primary Slave / index 1).
 */

#ifndef TINYOS_DISKIMG_H
#define TINYOS_DISKIMG_H

#include "uefi/types.h"

/* Magic signature stored at LBA 0 of disk0.img */
#define DISKIMG_MAGIC         "TINYOS_READY"
#define DISKIMG_MAGIC_LEN     12

/* Wallpaper location and dimensions */
#define DISKIMG_WALLPAPER_LBA    1000
#define DISKIMG_WALLPAPER_WIDTH  1024
#define DISKIMG_WALLPAPER_HEIGHT 768
#define DISKIMG_WALLPAPER_BPP    3       /* BGR24 = 3 bytes per pixel */
#define DISKIMG_WALLPAPER_SIZE   (DISKIMG_WALLPAPER_WIDTH * \
                                  DISKIMG_WALLPAPER_HEIGHT * \
                                  DISKIMG_WALLPAPER_BPP)
#define DISKIMG_WALLPAPER_SECTORS ((DISKIMG_WALLPAPER_SIZE + 511) / 512) /* 4608 */

/* Verify the magic signature at LBA 0 and load the wallpaper.
 * On verification failure, logs to serial and halts.
 * Returns false on fatal error (halts), true on success. */
bool diskimg_verify_and_load_wallpaper();

#endif /* TINYOS_DISKIMG_H */
