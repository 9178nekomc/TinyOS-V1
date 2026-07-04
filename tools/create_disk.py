"""TinyOS V1 — Disk Image Generator

Creates a 64MB raw binary disk image (disk0.img) with:
  - LBA 0:     Magic signature b"TINYOS_READY" (12 ASCII bytes)
  - LBA 1000:  1024x768 BGR24 wallpaper with centered 300x300 star-ring logo

Usage:
  python create_disk.py [output_path]

Default output: disk0.img in the current directory.
"""

import struct
import sys
import os

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DISK_SIZE_MB = 64
SECTOR_SIZE = 512
DISK_SIZE = DISK_SIZE_MB * 1024 * 1024

MAGIC = b"TINYOS_READY"
MAGIC_LBA = 0

WALLPAPER_WIDTH = 1024
WALLPAPER_HEIGHT = 768
WALLPAPER_BPP = 3  # BGR24
WALLPAPER_LBA = 1000
LOGO_SIZE = 300

# Calculate offsets
WALLPAPER_OFFSET = WALLPAPER_LBA * SECTOR_SIZE
WALLPAPER_SIZE = WALLPAPER_WIDTH * WALLPAPER_HEIGHT * WALLPAPER_BPP

# Logo centering
LOGO_X = (WALLPAPER_WIDTH - LOGO_SIZE) // 2   # 362
LOGO_Y = (WALLPAPER_HEIGHT - LOGO_SIZE) // 2  # 234


def generate_wallpaper():
    """Generate a 1024x768 BGR24 pixel matrix with a centered star-ring logo.

    Background: dark blue-black gradient (darker at edges, lighter in center).
    Logo: Concentric rings with radial spokes (star-ring design) in cyan/gold tones.

    Returns:
        bytes: 1024 * 768 * 3 bytes of BGR24 pixel data.
    """
    pixels = bytearray(WALLPAPER_SIZE)

    for y in range(WALLPAPER_HEIGHT):
        for x in range(WALLPAPER_WIDTH):
            offset = (y * WALLPAPER_WIDTH + x) * 3

            # Normalized coordinates relative to center
            cx = x - WALLPAPER_WIDTH / 2
            cy = y - WALLPAPER_HEIGHT / 2

            # Distance from center (0.0 at center, ~1.0 at corners)
            dx = cx / (WALLPAPER_WIDTH / 2)
            dy = cy / (WALLPAPER_HEIGHT / 2)
            dist = (dx * dx + dy * dy) ** 0.5

            # Background: dark blue gradient
            bg_b = int(15 + 40 * (1.0 - dist))   # Blue: 15-55
            bg_g = int(3 + 10 * (1.0 - dist))    # Green: 3-13
            bg_r = int(5 + 15 * (1.0 - dist))    # Red: 5-20

            # Star-field: sparse white dots
            star = 0
            seed = (x * 31337 + y * 7331) & 0xFFFFFFFF
            if (seed % 500) == 0:
                brightness = (seed % 156) + 100  # 100-255
                star = brightness

            b, g, r = bg_b, bg_g, bg_r

            # Logo region: centered 300x300
            lx = x - LOGO_X
            ly = y - LOGO_Y

            if 0 <= lx < LOGO_SIZE and 0 <= ly < LOGO_SIZE:
                # Coordinates relative to logo center (-150..150)
                lcx = lx - LOGO_SIZE / 2
                lcy = ly - LOGO_SIZE / 2
                ldist = (lcx * lcx + lcy * lcy) ** 0.5
                max_dist = LOGO_SIZE / 2  # 150

                # Normalized distance from logo center (0.0 at center, 1.0 at edges)
                ndist = ldist / max_dist

                # Angle for radial patterns
                import math
                angle = math.atan2(lcy, lcx)  # -pi to pi

                # Number of spokes
                num_spokes = 12
                spoke_angle = (angle + math.pi) % (2 * math.pi / num_spokes)
                spoke_width = 2 * math.pi / num_spokes * 0.15
                is_spoke = spoke_angle < spoke_width / 2 or spoke_angle > (2 * math.pi / num_spokes - spoke_width / 2)

                # Concentric rings
                ring_positions = [0.25, 0.45, 0.65, 0.85]
                ring_width = 0.04

                is_ring = False
                for rp in ring_positions:
                    if abs(ndist - rp) < ring_width:
                        is_ring = True
                        break

                # Outer circle boundary
                is_boundary = abs(ndist - 1.0) < 0.02

                # Inner circle
                is_inner = abs(ndist - 0.08) < 0.015

                if is_spoke or is_ring or is_boundary or is_inner:
                    # Star-ring element: cyan-gold gradient based on distance
                    t = ndist
                    logo_b = int(50 + 205 * (1.0 - t))
                    logo_g = int(180 + 75 * (1.0 - t))
                    logo_r = int(200 + 55 * t)

                    # Alpha blend with background
                    alpha = 0.95
                    b = int(b * (1 - alpha) + logo_b * alpha)
                    g = int(g * (1 - alpha) + logo_g * alpha)
                    r = int(r * (1 - alpha) + logo_r * alpha)

                    # Brighter at spoke/ring intersections and boundaries
                    if (is_spoke and is_ring) or is_boundary:
                        b = min(255, b + 40)
                        g = min(255, g + 30)
                        r = min(255, r + 20)

            # Add star overlay (sparse white dots everywhere)
            if star:
                b = min(255, b + star // 3)
                g = min(255, g + star // 3)
                r = min(255, r + star // 3)

            # Clamp
            b = max(0, min(255, b))
            g = max(0, min(255, g))
            r = max(0, min(255, r))

            # Store BGR
            pixels[offset + 0] = b
            pixels[offset + 1] = g
            pixels[offset + 2] = r

    return bytes(pixels)


def create_disk_image(output_path):
    """Create the full disk image."""
    print(f"Creating {DISK_SIZE_MB}MB disk image at: {output_path}")

    with open(output_path, 'wb') as f:
        # Pre-allocate the full 64MB
        print(f"  Pre-allocating {DISK_SIZE_MB}MB...")
        f.seek(DISK_SIZE - 1)
        f.write(b'\x00')

        # Write magic signature at LBA 0 (offset 0)
        print(f"  Writing magic signature at LBA {MAGIC_LBA}...")
        f.seek(0)
        f.write(MAGIC)
        # Pad rest of sector with zeros
        f.write(b'\x00' * (SECTOR_SIZE - len(MAGIC)))

        # Generate and write wallpaper at LBA 1000
        print(f"  Generating {WALLPAPER_WIDTH}x{WALLPAPER_HEIGHT} BGR24 wallpaper...")
        wallpaper = generate_wallpaper()
        print(f"  Wallpaper size: {len(wallpaper):,} bytes")
        print(f"  Sectors needed: {len(wallpaper) // SECTOR_SIZE} + {len(wallpaper) % SECTOR_SIZE} bytes remainder")

        f.seek(WALLPAPER_OFFSET)
        f.write(wallpaper)

        # Verify sizes
        if len(wallpaper) != WALLPAPER_SIZE:
            print(f"  WARNING: Wallpaper size mismatch: expected {WALLPAPER_SIZE}, got {len(wallpaper)}")

    # Verify
    file_size = os.path.getsize(output_path)
    print(f"\nDisk image created: {file_size:,} bytes ({file_size / 1024 / 1024:.1f}MB)")
    print(f"  LBA 0:     Magic = '{MAGIC.decode()}'")
    print(f"  LBA 1000:  Wallpaper ({WALLPAPER_WIDTH}x{WALLPAPER_HEIGHT} BGR24)")
    print(f"  Logo:      {LOGO_SIZE}x{LOGO_SIZE} star-ring at ({LOGO_X}, {LOGO_Y})")

    # Read back and verify magic
    with open(output_path, 'rb') as f:
        magic_read = f.read(len(MAGIC))
        assert magic_read == MAGIC, f"Magic verification failed: {magic_read}"
        print(f"  Verification: Magic OK")


if __name__ == '__main__':
    output = sys.argv[1] if len(sys.argv) > 1 else 'disk0.img'
    create_disk_image(output)
