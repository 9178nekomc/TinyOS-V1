/* TinyOS V1 — Graphics Output Protocol (GOP) Driver
 *
 * Locates the EFI_GRAPHICS_OUTPUT_PROTOCOL and provides framebuffer
 * access for rendering the wallpaper and logo.
 *
 * The framebuffer is stored as a global in .bss (NOT on the stack).
 */

#ifndef TINYOS_GOP_H
#define TINYOS_GOP_H

#include "uefi/types.h"

/* Framebuffer descriptor — global, in .bss, NOT stack */
struct Framebuffer {
    UINT8   *base;          /* FrameBufferBase virtual address */
    UINTN   size;           /* FrameBufferSize in bytes */
    UINT32  width;          /* HorizontalResolution */
    UINT32  height;         /* VerticalResolution */
    UINT32  stride;         /* PixelsPerScanLine */
    bool    is_bgr;         /* true = BGR24, false = RGB24 */
    UINT32  bytes_per_pixel; /* 3 for 24-bit, 4 for 32-bit */
};

extern Framebuffer g_fb;

/* Find GOP and store framebuffer info. Returns EFI_SUCCESS on success.
 * Falls back to the current mode if no 1024x768 mode is available. */
EFI_STATUS gop_init();

/* Render a 1024x768 BGR24 pixel buffer to the framebuffer.
 * Centers the image if the actual resolution is larger.
 * The pixel buffer is 1024*768*3 = 2,359,296 bytes. */
void gop_render_wallpaper(const UINT8 *bgr24_pixels);

/* Fill the entire screen with a solid BGR color. */
void gop_fill_screen(UINT8 blue, UINT8 green, UINT8 red);

#endif /* TINYOS_GOP_H */
