/* TinyOS V1 — Graphical Terminal Implementation */

#include "gfxterm.h"
#include "gop.h"
#include "font8x16.h"
#include "serial.h"
#include "alloc.h"

GfxTerm g_term;

/* ===========================================================================
 * Initialization
 * =========================================================================== */

bool gfxterm_init() {
    if (!g_fb.base || g_fb.width == 0 || g_fb.height == 0) {
        serial_print("[GFXTERM] Framebuffer not available\r\n");
        return false;
    }

    g_term.cols  = g_fb.width  / FONT_WIDTH;
    g_term.rows  = g_fb.height / FONT_HEIGHT;
    g_term.cur_x = 0;
    g_term.cur_y = 0;

    /* Colors: white on black */
    if (g_fb.is_bgr) {
        g_term.fg = 0x00FFFFFF;  /* BGR: white = 0xFF,0xFF,0xFF */
        g_term.bg = 0x00000000;  /* BGR: black */
    } else {
        g_term.fg = 0x00FFFFFF;  /* RGB: white */
        g_term.bg = 0x00000000;  /* RGB: black */
    }

    serial_print("[GFXTERM] ");
    serial_print_dec(g_term.cols);
    serial_print("x");
    serial_print_dec(g_term.rows);
    serial_print(" chars initialized\r\n");

    return true;
}

/* ===========================================================================
 * Drawing primitives
 * =========================================================================== */

void gfxterm_draw_char(int x, int y, char c, UINT32 fg, UINT32 bg) {
    if (!g_fb.base) return;
    if (x < 0 || x >= g_term.cols || y < 0 || y >= g_term.rows) return;
    if (c < 32 || c > 126) c = '?';

    int glyph_index = c - FONT_START;
    const unsigned char *glyph = &g_font8x16[glyph_index * FONT_HEIGHT];

    int px = x * FONT_WIDTH;
    int py = y * FONT_HEIGHT;
    UINT32 *fb = (UINT32*)g_fb.base;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            UINT32 color = (bits & (0x80 >> col)) ? fg : bg;
            fb[(py + row) * g_fb.stride + (px + col)] = color;
        }
    }
}

/* ===========================================================================
 * Screen management
 * =========================================================================== */

void gfxterm_clear() {
    if (!g_fb.base) return;

    UINT32 *fb = (UINT32*)g_fb.base;
    UINTN total = g_fb.stride * g_fb.height;
    for (UINTN i = 0; i < total; i++) {
        fb[i] = g_term.bg;
    }
    g_term.cur_x = 0;
    g_term.cur_y = 0;
}

void gfxterm_scroll() {
    if (!g_fb.base) return;

    /* Move each line up by one */
    UINT32 *fb = (UINT32*)g_fb.base;
    UINTN line_bytes = g_fb.stride * FONT_HEIGHT * sizeof(UINT32);
    UINTN total_bytes = g_fb.stride * g_fb.height * sizeof(UINT32);

    /* memmove: copy from line 1 to line 0 */
    UINT32 *dst = fb;
    UINT32 *src = fb + g_fb.stride * FONT_HEIGHT;
    UINTN copy_bytes = total_bytes - line_bytes;

    /* We must copy word by word to avoid issues */
    for (UINTN i = 0; i < (copy_bytes / sizeof(UINT32)); i++) {
        dst[i] = src[i];
    }

    /* Clear the last line */
    UINT32 *last = fb + g_fb.stride * (g_fb.height - FONT_HEIGHT);
    for (UINT32 y = 0; y < FONT_HEIGHT; y++) {
        for (UINT32 x = 0; x < g_fb.stride; x++) {
            last[y * g_fb.stride + x] = g_term.bg;
        }
    }
}

void gfxterm_set_cursor(int x, int y) {
    g_term.cur_x = x;
    g_term.cur_y = y;
}

/* ===========================================================================
 * Text output
 * =========================================================================== */

void gfxterm_putc(char c) {
    if (c == '\n') {
        g_term.cur_y++;
        g_term.cur_x = 0;
    } else if (c == '\r') {
        g_term.cur_x = 0;
    } else if (c == '\b') {
        if (g_term.cur_x > 0) {
            g_term.cur_x--;
            gfxterm_draw_char(g_term.cur_x, g_term.cur_y, ' ', g_term.fg, g_term.bg);
        }
    } else if (c == '\t') {
        g_term.cur_x = ((g_term.cur_x / 8) + 1) * 8;
    } else {
        /* Draw character */
        gfxterm_draw_char(g_term.cur_x, g_term.cur_y, c, g_term.fg, g_term.bg);
        g_term.cur_x++;
    }

    /* Line wrap */
    if (g_term.cur_x >= g_term.cols) {
        g_term.cur_x = 0;
        g_term.cur_y++;
    }

    /* Scroll if needed */
    while (g_term.cur_y >= g_term.rows) {
        gfxterm_scroll();
        g_term.cur_y--;
    }

    /* Also echo to serial for debugging */
    serial_putc(c);
}

void gfxterm_print(const char *str) {
    while (*str) {
        gfxterm_putc(*str++);
    }
}

void gfxterm_print_dec(UINT64 val) {
    if (val == 0) { gfxterm_putc('0'); return; }
    char buf[21];
    int i = 20;
    buf[i--] = '\0';
    while (val > 0) { buf[i--] = '0' + (val % 10); val /= 10; }
    gfxterm_print(&buf[i + 1]);
}

void gfxterm_print_hex(UINT64 val) {
    gfxterm_print("0x");
    if (val == 0) { gfxterm_putc('0'); return; }
    int s = 60;
    while (s >= 0 && ((val >> s) & 0xF) == 0) s -= 4;
    while (s >= 0) { UINT8 n = (val >> s) & 0xF; gfxterm_putc(n < 10 ? '0' + n : 'A' + n - 10); s -= 4; }
}

/* ===========================================================================
 * Line input with echo
 * =========================================================================== */

int gfxterm_readline(char *buf, int max_len) {
    int len = 0;

    while (1) {
        char c = 0;

        /* Poll serial (non-blocking) — primary input in -nographic mode */
        if (serial_can_read()) {
            c = serial_getc();
        }

        if (c == 0) {
            /* Also poll UEFI keyboard via CheckEvent */
            if (g_ST && g_ST->ConIn && g_ST->ConIn->WaitForKey) {
                EFI_STATUS st = g_BS->CheckEvent(g_ST->ConIn->WaitForKey);
                if (st == EFI_SUCCESS) {
                    EFI_INPUT_KEY key;
                    st = g_ST->ConIn->ReadKeyStroke(g_ST->ConIn, &key);
                    if (st == EFI_SUCCESS) {
                        UINT16 uc = key.UnicodeChar;
                        if (uc >= 0x20 && uc <= 0x7E) c = (char)uc;
                        else if (uc == '\r') c = '\r';
                        else if (uc == 0x08) c = '\b';
                    }
                }
            }
        }

        if (c == 0) {
            if (g_BS) g_BS->Stall(10000);
            continue;
        }

        /* Enter */
        if (c == '\r' || c == '\n') {
            gfxterm_putc('\n');
            buf[len] = '\0';
            return len;
        }

        /* Backspace */
        if (c == '\b' || c == 0x7F) {
            if (len > 0) {
                len--;
                gfxterm_putc('\b');
            }
            continue;
        }

        /* Ctrl+C */
        if (c == 0x03) {
            gfxterm_print("^C\n");
            buf[0] = '\0';
            return 0;
        }

        /* Printable */
        if (c >= 0x20 && c <= 0x7E && len < max_len - 1) {
            buf[len++] = c;
            gfxterm_putc(c);
        }
    }
}
