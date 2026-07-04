/* TinyOS V1 — Shell Commands (GFX Terminal + PFS) */

#include "commands.h"
#include "fat.h"
#include "gop.h"
#include "ata.h"
#include "alloc.h"
#include "serial.h"
#include "diskimg.h"
#include "gfxterm.h"
#include "pfs.h"
#include "install.h"

ShellCommand g_cmd_table[] = {
    {"ls",        "List directory (ESP)",               cmd_ls},
    {"cd",        "Change directory (ESP)",             cmd_cd},
    {"mkdir",     "Create directory (ESP)",              cmd_mkdir},
    {"pfls",      "List persistent files (System Disk)",cmd_pfls},
    {"mkfile",    "Create file on system disk",         cmd_mkfile},
    {"pfcat",     "Read file from system disk",         cmd_pfcat},
    {"pfrm",      "Delete file from system disk",       cmd_pfrm},
    {"pfwrite",   "Write text to system disk",          cmd_pfwrite},
    {"help",      "Show available commands",            cmd_help},
    {"clear",     "Clear the screen",                   cmd_clear},
    {"info",      "Display system information",         cmd_info},
    {"read",      "Read raw disk sectors (hex)",        cmd_read},
    {"wallpaper", "Re-render the wallpaper",            cmd_wallpaper},
    {"install",   "Install TinyOS to system disk",      cmd_install},
    {"shutdown",  "Shutdown / reboot the system",       cmd_shutdown},
    {nullptr,     nullptr,                              nullptr}
};

int g_cmd_count = 15;

/* ---- ls ---- */
EFI_STATUS cmd_ls(int argc, char **argv) {
    return fat_ls((argc > 1) ? argv[1] : nullptr);
}
/* ---- cd ---- */
EFI_STATUS cmd_cd(int argc, char **argv) {
    if (argc < 2) { gfxterm_print("Usage: cd <path>\n"); return EFI_INVALID_PARAMETER; }
    return fat_cd(argv[1]);
}
/* ---- mkdir ---- */
EFI_STATUS cmd_mkdir(int argc, char **argv) {
    if (argc < 2) { gfxterm_print("Usage: mkdir <name>\n"); return EFI_INVALID_PARAMETER; }
    return fat_mkdir(argv[1]);
}

/* ---- pfls (persistent file list) ---- */
EFI_STATUS cmd_pfls(int argc, char **argv) {
    (void)argc; (void)argv;
    if (!pfs_is_mounted()) {
        gfxterm_print("  No system disk mounted. Install TinyOS first.\n");
        return EFI_NOT_STARTED;
    }
    pfs_list();
    return EFI_SUCCESS;
}

/* ---- mkfile (create persistent file) ---- */
EFI_STATUS cmd_mkfile(int argc, char **argv) {
    if (argc < 2) { gfxterm_print("Usage: mkfile <name>\n"); return EFI_INVALID_PARAMETER; }
    if (!pfs_is_mounted()) { gfxterm_print("  System disk not mounted.\n"); return EFI_NOT_STARTED; }
    if (pfs_create(argv[1], false))
        gfxterm_print("  File created.\n");
    else
        gfxterm_print("  Failed (already exists or disk full).\n");
    return EFI_SUCCESS;
}

/* ---- pfcat (read persistent file) ---- */
EFI_STATUS cmd_pfcat(int argc, char **argv) {
    if (argc < 2) { gfxterm_print("Usage: pfcat <name>\n"); return EFI_INVALID_PARAMETER; }
    if (!pfs_is_mounted()) { gfxterm_print("  System disk not mounted.\n"); return EFI_NOT_STARTED; }
    UINT8 buf[1024];
    UINT32 sz = pfs_read_file(argv[1], buf, sizeof(buf) - 1);
    if (sz == 0) { gfxterm_print("  File not found or empty.\n"); return EFI_NOT_FOUND; }
    buf[sz] = '\0';
    gfxterm_print((const char*)buf);
    gfxterm_print("\n");
    return EFI_SUCCESS;
}

/* ---- pfrm (delete persistent file) ---- */
EFI_STATUS cmd_pfrm(int argc, char **argv) {
    if (argc < 2) { gfxterm_print("Usage: pfrm <name>\n"); return EFI_INVALID_PARAMETER; }
    if (!pfs_is_mounted()) { gfxterm_print("  System disk not mounted.\n"); return EFI_NOT_STARTED; }
    if (pfs_delete(argv[1]))
        gfxterm_print("  Deleted.\n");
    else
        gfxterm_print("  File not found.\n");
    return EFI_SUCCESS;
}

/* ---- pfwrite (write text to persistent file) ---- */
EFI_STATUS cmd_pfwrite(int argc, char **argv) {
    if (argc < 3) { gfxterm_print("Usage: pfwrite <name> <text>\n"); return EFI_INVALID_PARAMETER; }
    if (!pfs_is_mounted()) { gfxterm_print("  System disk not mounted.\n"); return EFI_NOT_STARTED; }
    /* Join remaining args as the text */
    /* Simple: just use argv[2] for now */
    if (pfs_write_file(argv[1], (const UINT8*)argv[2],
        (UINT32)(argv[2] ? strlen_efi(argv[2]) : 0)))
        gfxterm_print("  Written.\n");
    else
        gfxterm_print("  Write failed (disk full?)\n");
    return EFI_SUCCESS;
}

/* ---- help ---- */
EFI_STATUS cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    gfxterm_print("\n  TinyOS V1 -- Available Commands\n");
    gfxterm_print("  ================================\n\n");
    for (int i = 0; g_cmd_table[i].name; i++) {
        gfxterm_print("  ");
        gfxterm_print(g_cmd_table[i].name);
        int len = 0; const char *n = g_cmd_table[i].name; while (*n++) len++;
        for (int s = len; s < 14; s++) gfxterm_putc(' ');
        gfxterm_print(g_cmd_table[i].help);
        gfxterm_print("\n");
    }
    gfxterm_print("\n");
    return EFI_SUCCESS;
}

/* ---- clear ---- */
EFI_STATUS cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv; gfxterm_clear(); return EFI_SUCCESS;
}

/* ---- info ---- */
EFI_STATUS cmd_info(int argc, char **argv) {
    (void)argc; (void)argv;
    gfxterm_print("\n  TinyOS V1 -- System Information\n");
    gfxterm_print("  ==============================\n\n");
    gfxterm_print("  Display  : "); gfxterm_print_dec(g_fb.width);
    gfxterm_print("x"); gfxterm_print_dec(g_fb.height);
    gfxterm_print(g_fb.is_bgr ? " BGR32\n" : " RGB32\n");
    gfxterm_print("  Terminal : "); gfxterm_print_dec(g_term.cols);
    gfxterm_print("x"); gfxterm_print_dec(g_term.rows); gfxterm_print(" chars\n");
    gfxterm_print("  ESP      : "); gfxterm_print(g_cwd); gfxterm_print("\n");
    gfxterm_print("  PFS      : ");
    if (pfs_is_mounted()) {
        gfxterm_print("mounted, "); gfxterm_print_dec(pfs_free_blocks());
        gfxterm_print(" blocks free\n");
    } else { gfxterm_print("not mounted\n"); }
    gfxterm_print("\n");
    return EFI_SUCCESS;
}

/* ---- read ---- */
EFI_STATUS cmd_read(int argc, char **argv) {
    if (argc < 2) { gfxterm_print("Usage: read <lba> [count]\n"); return EFI_INVALID_PARAMETER; }
    UINT64 lba = 0; const char *s = argv[1];
    while (*s >= '0' && *s <= '9') { lba = lba * 10 + (*s - '0'); s++; }
    UINT16 count = 1;
    if (argc > 2) { count = 0; s = argv[2];
        while (*s >= '0' && *s <= '9') { count = count * 10 + (*s - '0'); s++; }
        if (count == 0) count = 1; if (count > 4) count = 4; }
    UINT8 *buf = (UINT8*)os_alloc(count * 512);
    if (EFI_ERROR(ata_read(lba, count, buf, ATA_DRIVE_SLAVE))) {
        gfxterm_print("Read failed.\n"); os_free(buf); return EFI_DEVICE_ERROR;
    }
    gfxterm_print("LBA "); gfxterm_print_dec(lba); gfxterm_print("\n");
    UINT8 *sec = buf;
    for (int row = 0; row < 32; row++) {
        gfxterm_print_hex((UINT64)(row*16)); gfxterm_print(": ");
        for (int col = 0; col < 16; col++) {
            UINT8 b = sec[row*16+col];
            gfxterm_putc("0123456789ABCDEF"[b>>4]);
            gfxterm_putc("0123456789ABCDEF"[b&0xF]); gfxterm_putc(' ');
        }
        gfxterm_print("|");
        for (int col = 0; col < 16; col++) {
            UINT8 b = sec[row*16+col];
            gfxterm_putc((b>=0x20&&b<=0x7E)?(char)b:'.');
        }
        gfxterm_print("|\n");
    }
    os_free(buf); return EFI_SUCCESS;
}

/* ---- wallpaper ---- */
EFI_STATUS cmd_wallpaper(int argc, char **argv) {
    (void)argc; (void)argv;
    gfxterm_print("Loading wallpaper...\n");
    UINT8 *wp = (UINT8*)os_alloc(DISKIMG_WALLPAPER_SIZE);
    UINT8 *dst = wp; UINT16 rem = DISKIMG_WALLPAPER_SECTORS;
    UINT64 lba = DISKIMG_WALLPAPER_LBA;
    while (rem > 0) { UINT16 n = (rem > 128) ? 128 : rem;
        ata_read(lba, n, dst, ATA_DRIVE_SLAVE); dst += n*512; lba += n; rem -= n; }
    gop_render_wallpaper(wp); os_free(wp);
    gfxterm_print("Done. Press Enter...\n");
    char dummy[8]; gfxterm_readline(dummy, sizeof(dummy)); gfxterm_clear();
    return EFI_SUCCESS;
}

/* ---- install ---- */
EFI_STATUS cmd_install(int argc, char **argv) {
    (void)argc; (void)argv;
    bool ok = install_run();
    if (ok) {
        gfxterm_print("\n  Installation complete! Rebooting...\n");
        for (volatile int i = 0; i < 30000000; i++) __asm__ volatile("pause");
        /* Signal main to reboot */
    }
    return EFI_SUCCESS;
}

/* ---- shutdown ---- */
EFI_STATUS cmd_shutdown(int argc, char **argv) {
    (void)argc; (void)argv;
    gfxterm_print("\n  Shutting down...\n");
    for (volatile int i = 0; i < 10000000; i++) __asm__ volatile("pause");
    /* Use runtime reset via function pointer cast.
     * Signature: EFI_STATUS EFIAPI ResetSystem(INTN, EFI_STATUS, UINTN, VOID*) */
    if (g_ST && g_ST->RuntimeServices) {
        typedef EFI_STATUS (EFIAPI *ResetFn)(INTN, EFI_STATUS, UINTN, VOID*);
        ResetFn reset = (ResetFn)(UINTN)g_ST->RuntimeServices->ResetSystem;
        reset(0 /* EfiResetCold */, EFI_SUCCESS, 0, nullptr);
    }
    /* If that returned, fallback */
    return EFI_SUCCESS;
}
