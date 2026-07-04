/* TinyOS V1 — Embedded 8x16 Bitmap Font
 *
 * Covers ASCII 32 (space) through 126 (~), 95 glyphs × 16 bytes = 1520 bytes.
 * This is a standard VGA-style 8-pixel-wide, 16-pixel-tall monospace font.
 * Each glyph is stored column-major: first byte = top row, last byte = bottom row.
 * A '1' bit means lit pixel, '0' means background.
 */

#ifndef TINYOS_FONT8X16_H
#define TINYOS_FONT8X16_H

/* Font metrics */
#define FONT_WIDTH   8
#define FONT_HEIGHT  16
#define FONT_START   32   /* First character in the font */
#define FONT_COUNT   95   /* Number of glyphs (32-126) */

extern const unsigned char g_font8x16[FONT_COUNT * FONT_HEIGHT];

#endif /* TINYOS_FONT8X16_H */
