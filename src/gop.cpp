/* TinyOS V1 — GOP Driver Implementation */

#include "gop.h"
#include "alloc.h"
#include "serial.h"

/* Global framebuffer state — lives in .bss */
Framebuffer g_fb;

/* The GOP protocol instance we're using */
static EFI_GRAPHICS_OUTPUT_PROTOCOL *g_gop = nullptr;

/* The GUID for EFI_GRAPHICS_OUTPUT_PROTOCOL */
static const EFI_GUID gEfiGraphicsOutputProtocolGuid =
    EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

/* ===========================================================================
 * Public API
 * =========================================================================== */

EFI_STATUS gop_init() {
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = nullptr;

    serial_print("[GOP] Locating graphics output protocol...\r\n");

    /* Locate the GOP */
    status = g_BS->LocateProtocol(
        (EFI_GUID*)&gEfiGraphicsOutputProtocolGuid,
        nullptr,
        (VOID**)&gop
    );

    if (EFI_ERROR(status) || gop == nullptr) {
        serial_print("[GOP] ERROR: Cannot find GOP, status=");
        serial_print_hex(status);
        serial_print("\r\n");
        return status;
    }

    g_gop = gop;

    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = gop->Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = mode->Info;

    serial_print("[GOP] Current mode: ");
    serial_print_dec(info->HorizontalResolution);
    serial_print("x");
    serial_print_dec(info->VerticalResolution);
    serial_print(" pixel_format=");
    serial_print_dec(info->PixelFormat);
    serial_print(" stride=");
    serial_print_dec(info->PixelsPerScanLine);
    serial_print("\r\n");

    /* Try to find a 1024x768 mode if available */
    UINT32 best_mode = mode->Mode;  /* Keep current by default */
    bool found_1024x768 = false;

    for (UINT32 i = 0; i < mode->MaxMode; i++) {
        UINTN size_of_info = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mode_info = nullptr;

        status = gop->QueryMode(gop, i, &size_of_info, &mode_info);
        if (EFI_ERROR(status)) continue;

        if (mode_info->HorizontalResolution == 1024 &&
            mode_info->VerticalResolution == 768 &&
            (mode_info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor ||
             mode_info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)) {
            best_mode = i;
            found_1024x768 = true;
            serial_print("[GOP] Found 1024x768 mode at index ");
            serial_print_dec(i);
            serial_print("\r\n");
            break;
        }
    }

    /* Set the chosen mode */
    if (found_1024x768) {
        status = gop->SetMode(gop, best_mode);
        if (EFI_ERROR(status)) {
            serial_print("[GOP] WARNING: SetMode(1024x768) failed, keeping current\r\n");
        } else {
            serial_print("[GOP] SetMode(1024x768) OK\r\n");
        }
    }

    /* Read back the (possibly new) mode info */
    mode = gop->Mode;
    info = mode->Info;

    /* Fill the global framebuffer descriptor */
    g_fb.base   = (UINT8*)(UINTN)mode->FrameBufferBase;
    g_fb.size   = mode->FrameBufferSize;
    g_fb.width  = info->HorizontalResolution;
    g_fb.height = info->VerticalResolution;
    g_fb.stride = info->PixelsPerScanLine;
    g_fb.is_bgr = (info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor);
    g_fb.bytes_per_pixel = 4; /* UEFI always uses 32-bit per pixel in framebuffer */

    serial_print("[GOP] Framebuffer: ");
    serial_print_hex((UINT64)(UINTN)g_fb.base);
    serial_print(" size=");
    serial_print_dec(g_fb.size);
    serial_print(" ");
    serial_print_dec(g_fb.width);
    serial_print("x");
    serial_print_dec(g_fb.height);
    serial_print(g_fb.is_bgr ? " BGR32" : " RGB32");
    serial_print("\r\n");

    /* Ensure framebuffer is valid */
    if (g_fb.base == nullptr || g_fb.size == 0) {
        serial_print("[GOP] ERROR: Invalid framebuffer\r\n");
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}

void gop_render_wallpaper(const UINT8 *bgr24_pixels) {
    if (g_fb.base == nullptr || bgr24_pixels == nullptr) {
        serial_print("[GOP] ERROR: Cannot render wallpaper — no framebuffer or pixels\r\n");
        return;
    }

    serial_print("[GOP] Rendering wallpaper...\r\n");

    /* Calculate centering offsets if framebuffer > 1024x768 */
    UINT32 offset_x = (g_fb.width  > 1024) ? (g_fb.width  - 1024) / 2 : 0;
    UINT32 offset_y = (g_fb.height > 768)  ? (g_fb.height - 768)  / 2 : 0;
    UINT32 copy_w    = (g_fb.width  < 1024) ? g_fb.width  : 1024;
    UINT32 copy_h    = (g_fb.height < 768)  ? g_fb.height : 768;

    /* For performance, use a direct copy loop.
     * Source: 24-bit BGR (3 bytes per pixel), bottom-up or top-down?
     * We assume top-down (row 0 = top of image).
     * Destination: 32-bit XRGB framebuffer. */

    for (UINT32 y = 0; y < copy_h; y++) {
        for (UINT32 x = 0; x < copy_w; x++) {
            UINT32 src_offset = (y * 1024 + x) * 3;
            UINT8 b = bgr24_pixels[src_offset + 0];
            UINT8 g = bgr24_pixels[src_offset + 1];
            UINT8 r = bgr24_pixels[src_offset + 2];

            UINT32 dst_x = x + offset_x;
            UINT32 dst_y = y + offset_y;
            UINT32 dst_offset = dst_y * g_fb.stride + dst_x;

            UINT32 *fb32 = (UINT32*)g_fb.base;
            if (g_fb.is_bgr) {
                /* Framebuffer is BGRx — B=LSB, G, R, x=MSB */
                fb32[dst_offset] = (b) | (g << 8) | (r << 16);
            } else {
                /* Framebuffer is RGBx — R=LSB, G, B, x=MSB */
                fb32[dst_offset] = (r) | (g << 8) | (b << 16);
            }
        }
    }

    serial_print("[GOP] Wallpaper rendered.\r\n");
}

void gop_fill_screen(UINT8 blue, UINT8 green, UINT8 red) {
    if (g_fb.base == nullptr) return;

    UINT32 color;
    if (g_fb.is_bgr) {
        color = (blue) | (green << 8) | (red << 16);
    } else {
        color = (red) | (green << 8) | (blue << 16);
    }

    UINT32 *fb32 = (UINT32*)g_fb.base;
    UINTN total_pixels = (UINTN)g_fb.stride * g_fb.height;
    for (UINTN i = 0; i < total_pixels; i++) {
        fb32[i] = color;
    }
}
