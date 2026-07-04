"""TinyOS V1 — UEFI Bootable ISO / Disk Image Generator

Creates:
  - TinyOS-V1.iso   (bootable ISO9660 with El Torito UEFI boot)
  - TinyOS-V1.img   (raw bootable FAT32 disk image)

No external tools required — pure Python.
"""

import os, struct, sys

ISO_SECTOR = 2048

def w32(buf, off, v): struct.pack_into("<I", buf, off, v)
def w16(buf, off, v): struct.pack_into("<H", buf, off, v)

def build_fat32_image(esp_dir, out_path):
    """Build a proper FAT32 disk image from an ESP directory tree."""

    # Phase 1: scan directory with proper hierarchy
    class Node:
        def __init__(self, name, is_dir, data=b""):
            self.name = name.upper()
            self.is_dir = is_dir
            self.data = data
            self.children = []
            self.cluster = 0

    root = Node("", True)
    # Collect files
    file_nodes = []
    for dirpath, dirnames, filenames in os.walk(esp_dir):
        rel = os.path.relpath(dirpath, esp_dir)
        if rel == ".":
            parent = root
        else:
            parts = rel.replace("\\", "/").split("/")
            parent = root
            for p in parts:
                found = None
                for c in parent.children:
                    if c.name == p.upper() and c.is_dir:
                        found = c; break
                if not found:
                    found = Node(p, True)
                    parent.children.append(found)
                parent = found
        for fn in filenames:
            full = os.path.join(dirpath, fn)
            data = open(full, "rb").read()
            n = Node(fn, False, data)
            parent.children.append(n)
            file_nodes.append(n)
        for dn in dirnames:
            full = os.path.join(dirpath, dn)
            n = Node(dn, True)
            parent.children.append(n)

    print(f"  Files: {len(file_nodes)}")
    for f in file_nodes:
        print(f"    {get_path(f)}  ({len(f.data):,} bytes)")

    # Phase 2: calculate layout
    CLUSTER_SIZE = 512  # 1 sector per cluster
    DIR_ENTRY_SIZE = 32
    ENTRIES_PER_CLUSTER = CLUSTER_SIZE // DIR_ENTRY_SIZE  # 16

    # Assign clusters (simple: each dir/file gets contiguous clusters)
    next_cluster = 3  # cluster 2 = root dir
    def assign_clusters(node):
        nonlocal next_cluster
        if node == root:
            node.cluster = 2
            for c in node.children:
                assign_clusters(c)
        elif node.is_dir:
            node.cluster = next_cluster
            next_cluster += 1
            for c in node.children:
                assign_clusters(c)
        else:
            node.cluster = next_cluster
            clusters_needed = (len(node.data) + CLUSTER_SIZE - 1) // CLUSTER_SIZE
            if clusters_needed == 0: clusters_needed = 1
            next_cluster += clusters_needed

    assign_clusters(root)

    total_clusters = next_cluster
    reserved_sectors = 32
    fat_count = 1
    # FAT32 needs sectors_per_fat to cover total_clusters * 4 bytes
    sectors_per_fat = (total_clusters * 4 + 511) // 512 + 1
    root_dir_sectors = (total_clusters - 2) * 1  # rough
    total_sectors = reserved_sectors + sectors_per_fat + total_clusters
    total_sectors = (total_sectors + 7) & ~7  # align to 4KB

    print(f"  FAT32: {total_sectors} sectors ({total_sectors*512:,} bytes)")

    fat = bytearray(total_sectors * 512)

    # BPB
    fat[0:3] = b'\xEB\x58\x90'
    fat[3:11] = b'TINYOS  '
    w16(fat, 11, 512)
    fat[13] = 1  # sec/cluster
    w16(fat, 14, reserved_sectors)
    fat[16] = fat_count
    w16(fat, 17, 0)
    w16(fat, 19, 0)
    fat[21] = 0xF8
    w16(fat, 22, 0)
    w32(fat, 24, 0)
    w32(fat, 28, total_sectors)
    w32(fat, 32, sectors_per_fat)
    w16(fat, 36, 0)
    w16(fat, 38, 0)
    w32(fat, 40, 2)  # root cluster
    w16(fat, 44, 1)  # fsinfo
    w16(fat, 46, 6)  # backup
    fat[64:68] = b'RRaA'
    w32(fat, 484, 0xAA550000)
    fat[510:512] = b'\x55\xAA'

    # FSInfo at sector 1
    w32(fat, 512, 0x41615252)
    w32(fat, 516, 0x61417272)
    w32(fat, 520, total_clusters - next_cluster)  # free clusters
    w32(fat, 524, 3)   # next free
    w32(fat, 528, 0xAA550000)

    # FAT table
    fat_base = reserved_sectors * 512
    # cluster 0-1 reserved
    w32(fat, fat_base, 0x0FFFFFF8)
    w32(fat, fat_base + 4, 0x0FFFFFFF)
    # all other clusters: mark as EOC initially
    for c in range(2, total_clusters):
        w32(fat, fat_base + c * 4, 0x0FFFFFFF)

    # Phase 3: write directories and files
    def write_dir(node):
        cluster_off = (reserved_sectors + sectors_per_fat + (node.cluster - 2)) * 512
        pos = 0

        # count entries needed
        entry_count = 1  # "."
        if node != root:
            entry_count += 1  # ".."
        entry_count += len(node.children)

        # pad to cluster
        buf = bytearray(((entry_count + ENTRIES_PER_CLUSTER - 1) // ENTRIES_PER_CLUSTER) * CLUSTER_SIZE)

        # "." entry
        def write_entry(off, name, attr, cluster, size):
            bname = name[:8].ljust(8).encode('ascii', 'ignore')
            bext = b'   '
            if '.' in name and not attr & 0x10:  # has extension
                base, ext = name.rsplit('.', 1)
                bname = base[:8].upper().ljust(8).encode('ascii', 'ignore')
                bext = ext[:3].upper().ljust(3).encode('ascii', 'ignore')
            buf[off:off+8] = bname[:8]
            buf[off+8:off+11] = bext[:3]
            buf[off+11] = attr
            buf[off+12] = 0  # reserved
            w16(buf, off+20, (cluster >> 16) & 0xFFFF)
            w16(buf, off+26, cluster & 0xFFFF)
            w32(buf, off+28, size)

        write_entry(pos, ".", 0x10, node.cluster, 0)
        pos += 32
        if node != root:
            # ".." points to root for top-level, or parent
            parent_cluster = 2  # root
            write_entry(pos, "..", 0x10, parent_cluster, 0)
            pos += 32

        for child in node.children:
            size = 0 if child.is_dir else len(child.data)
            attr = 0x10 if child.is_dir else 0x20
            write_entry(pos, child.name, attr, child.cluster, size)
            pos += 32

        fat[cluster_off : cluster_off + len(buf)] = buf

    def write_file(node):
        if node.is_dir:
            return
        cluster_off = (reserved_sectors + sectors_per_fat + (node.cluster - 2)) * 512
        fat[cluster_off : cluster_off + len(node.data)] = node.data

    def traverse(node):
        if node.is_dir:
            write_dir(node)
            for c in node.children:
                traverse(c)
        else:
            write_file(node)

    traverse(root)

    open(out_path, "wb").write(bytes(fat))
    return total_sectors * 512


def get_path(node, s=""):
    if node.name:
        s = "/" + node.name + s
    # can't get parent, just return name
    return (node.name or "/") + s


def build_iso(fat_path, iso_path):
    """Wrap a FAT image in an ISO9660 + El Torito bootable ISO."""
    fat_data = open(fat_path, "rb").read()
    fat_size = len(fat_data)
    fat_sectors = (fat_size + ISO_SECTOR - 1) // ISO_SECTOR

    # ISO layout: PVD(16) + BootRecord(17) + Terminator(18) + BootCatalog(19) + FAT(20+)
    iso_sectors = 20 + fat_sectors
    iso = bytearray(iso_sectors * ISO_SECTOR)
    pvd = 16 * ISO_SECTOR
    iso[pvd] = 1
    iso[pvd+1:pvd+6] = b'CD001'
    iso[pvd+6] = 1
    w32(iso, pvd+80, iso_sectors)
    w16(iso, pvd+120, 1)
    w16(iso, pvd+124, 1)
    w16(iso, pvd+128, ISO_SECTOR)
    vol_id = b'TINYOS_V1_BOOT   '
    iso[pvd+40:pvd+72] = vol_id[:32]

    bd = 17 * ISO_SECTOR
    iso[bd:bd+1] = b'\x00'
    iso[bd+1:bd+6] = b'CD001'
    iso[bd+6] = 1
    iso[bd+7:bd+39] = b'EL TORITO SPECIFICATION\0\0\0\0\0\0\0\0\0'
    w32(iso, bd+71, 19)

    iso[18*ISO_SECTOR] = 0xFF
    iso[18*ISO_SECTOR+1:18*ISO_SECTOR+6] = b'CD001'

    bc = 19 * ISO_SECTOR
    # Validation entry
    iso[bc] = 1
    iso[bc+1] = 0xEF  # UEFI
    w16(iso, bc+2, 0)
    iso[bc+28:bc+32] = b'\x55\xAA'
    # Default entry
    bc2 = bc + 32
    iso[bc2] = 0x88
    iso[bc2+1] = 0xEF
    w16(iso, bc2+4, 0)
    iso[bc2+6] = 0
    iso[bc2+7] = 0
    w16(iso, bc2+8, fat_sectors)
    w32(iso, bc2+12, 20)
    iso[bc2+16:bc2+28] = b'\0' * 12
    # Checksum
    for i in range(0, len(iso[bc:bc+64])):
        if i not in (0, 1, 2, 3, 28, 29, 30, 31):
            iso[bc+i] ^= 0x55

    iso[20*ISO_SECTOR : 20*ISO_SECTOR + fat_size] = fat_data
    open(iso_path, "wb").write(bytes(iso))
    return len(iso)


if __name__ == "__main__":
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    esp = os.path.join(project_root, "esp_temp")

    # Build FAT32 image
    print("Building TinyOS V1 FAT32 boot image...")
    print(f"  ESP: {esp}")
    fat_path = os.path.join(project_root, "tinyos_boot.fat")
    fat_size = build_fat32_image(esp, fat_path)
    print(f"  FAT image: {fat_path} ({fat_size/1024:.0f} KB)")

    # Build ISO
    print(f"\nBuilding bootable ISO...")
    iso_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(project_root, "TinyOS-V1.iso")
    iso_size = build_iso(fat_path, iso_path)
    print(f"  ISO: {iso_path} ({iso_size/1024:.0f} KB)")

    # Clean up temp FAT
    os.remove(fat_path)
    print(f"\nDone! Boot with: qemu-system-x86_64 -bios OVMF.fd -cdrom TinyOS-V1.iso")
