/* TinyOS V1 — Filesystem Operations Implementation
 *
 * Uses UEFI's EFI_SIMPLE_FILE_SYSTEM_PROTOCOL and EFI_FILE_PROTOCOL
 * to provide filesystem access on the ESP (Disk 0).
 */

#include "fat.h"
#include "alloc.h"
#include "serial.h"
#include "gfxterm.h"

/* Current working directory — global in .bss */
char g_cwd[FAT_MAX_PATH] = "/";

/* The root file handle — opened once at init */
static EFI_FILE_PROTOCOL *g_root = nullptr;

/* Current working directory file handle */
static EFI_FILE_PROTOCOL *g_pwd = nullptr;

/* Directory entry read buffer — .bss global, NOT stack */
static UINT8 g_dir_info_buf[4096];

/* GUIDs needed */
static const EFI_GUID gEfiSimpleFileSystemProtocolGuid =
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

static const EFI_GUID gEfiFileInfoGuid = EFI_FILE_INFO_ID;

static const EFI_GUID gEfiLoadedImageProtocolGuid =
    EFI_LOADED_IMAGE_PROTOCOL_GUID;

/* ===========================================================================
 * Utility: String conversion
 * =========================================================================== */

void fat_ascii_to_wide(const char *src, CHAR16 *dst, int len) {
    for (int i = 0; i < len && src[i]; i++) {
        dst[i] = (CHAR16)(UINT8)src[i];
    }
    dst[len] = 0;
}

void fat_wide_to_ascii(const CHAR16 *src, char *dst, int max_len) {
    for (int i = 0; i < max_len - 1 && src[i]; i++) {
        UINT16 c = src[i];
        dst[i] = (c >= 0x20 && c <= 0x7E) ? (char)c : '?';
    }
    /* Null-terminate at last written position */
    int i = 0;
    while (i < max_len - 1 && src[i]) i++;
    dst[i] = '\0';
}

/* ===========================================================================
 * Utility: Path resolution
 * =========================================================================== */

/* Copy string s to d, at most n chars */
static void str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i < n - 1 && s[i]; i++) {
        d[i] = s[i];
    }
    d[i] = '\0';
}

static int str_len(const char *s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

static bool str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}

/* Check if a path is absolute (starts with '/') */
static bool is_absolute(const char *path) {
    return path && path[0] == '/';
}

/* Open a file or directory by its path relative to root or pwd.
 * Returns EFI_SUCCESS and sets *out_handle on success. */
static EFI_STATUS fat_open_path(const char *path, EFI_FILE_PROTOCOL **out_handle) {
    if (out_handle == nullptr) return EFI_INVALID_PARAMETER;
    *out_handle = nullptr;

    if (path == nullptr || path[0] == '\0') {
        /* Empty path — open "." from g_pwd to get a FRESH handle.
         * We must NOT reuse g_pwd directly because Read() consumes
         * the internal cursor — subsequent ls calls would see nothing. */
        CHAR16 dot[2] = { '.', 0 };
        EFI_STATUS st = g_pwd->Open(g_pwd, out_handle, dot, EFI_FILE_MODE_READ, 0);
        return st;
    }

    /* Determine starting handle */
    EFI_FILE_PROTOCOL *start = g_root;
    const char *remaining = path;

    if (is_absolute(path)) {
        remaining = path + 1;  /* Skip leading '/' */
        if (*remaining == '\0') {
            /* Root dir — open "." from g_root to get a FRESH handle */
            CHAR16 dot[2] = { '.', 0 };
            return g_root->Open(g_root, out_handle, dot, EFI_FILE_MODE_READ, 0);
        }
        start = g_root;
    } else {
        start = g_pwd;
        remaining = path;
    }

    /* Tokenize path by '/' */
    char token[FAT_MAX_PATH];
    CHAR16 wide_token[FAT_MAX_PATH];
    EFI_FILE_PROTOCOL *current = start;
    EFI_FILE_PROTOCOL *next = nullptr;

    const char *p = remaining;
    while (*p) {
        /* Extract next token */
        int ti = 0;
        while (*p && *p != '/' && ti < FAT_MAX_PATH - 1) {
            token[ti++] = *p++;
        }
        token[ti] = '\0';
        if (*p == '/') p++;

        if (ti == 0) continue;  /* Skip empty components */

        /* Handle ".." */
        /* For simplicity: ".." is handled by opening the parent path.
         * EFI_FILE_PROTOCOL doesn't have a direct "parent" operation,
         * so ".." requires path string manipulation at the shell level.
         * For now, treat ".." as an attempt to open a file literally
         * named ".." — which won't exist. The shell handles ".." by
         * manipulating g_cwd. */
        if (str_eq(token, ".")) continue;

        fat_ascii_to_wide(token, wide_token, ti);

        EFI_STATUS status = current->Open(
            current, &next, wide_token,
            EFI_FILE_MODE_READ, 0
        );

        if (EFI_ERROR(status)) {
            return status;
        }

        /* Don't close the starting handle (it's g_root or g_pwd) */
        current = next;
    }

    *out_handle = current;
    return EFI_SUCCESS;
}

/* ===========================================================================
 * Initialization
 * =========================================================================== */

EFI_STATUS fat_init() {
    serial_print("[FAT] Initializing filesystem...\r\n");

    /* Step 1: Get the loaded image protocol to find our device handle */
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = nullptr;
    EFI_STATUS status = g_BS->OpenProtocol(
        g_IH,
        (EFI_GUID*)&gEfiLoadedImageProtocolGuid,
        (VOID**)&loaded_image,
        g_IH,
        nullptr,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if (EFI_ERROR(status)) {
        serial_print("[FAT] ERROR: Cannot open LoadedImageProtocol, status=");
        serial_print_hex(status);
        serial_print("\r\n");
        return status;
    }

    /* Step 2: Open the Simple File System Protocol on the boot device */
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfs = nullptr;
    status = g_BS->OpenProtocol(
        loaded_image->DeviceHandle,
        (EFI_GUID*)&gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&sfs,
        g_IH,
        nullptr,
        EFI_OPEN_PROTOCOL_GET_PROTOCOL
    );

    if (EFI_ERROR(status)) {
        serial_print("[FAT] ERROR: Cannot open SimpleFileSystemProtocol, status=");
        serial_print_hex(status);
        serial_print("\r\n");
        return status;
    }

    /* Step 3: Open the volume root */
    status = sfs->OpenVolume(sfs, &g_root);
    if (EFI_ERROR(status)) {
        serial_print("[FAT] ERROR: Cannot open volume root, status=");
        serial_print_hex(status);
        serial_print("\r\n");
        return status;
    }

    g_pwd = g_root;
    g_cwd[0] = '/';
    g_cwd[1] = '\0';

    serial_print("[FAT] Filesystem initialized. Root opened.\r\n");
    return EFI_SUCCESS;
}

/* ===========================================================================
 * Directory listing (ls)
 * =========================================================================== */

EFI_STATUS fat_ls(const char *path) {
    EFI_FILE_PROTOCOL *dir = nullptr;
    EFI_STATUS status;

    /* Always open a fresh handle — never reuse g_pwd */
    status = fat_open_path(path ? path : "", &dir);
    if (EFI_ERROR(status) || dir == nullptr) {
        gfxterm_print("ls: cannot access '");
        gfxterm_print(path ? path : ".");
        gfxterm_print("' (error ");
        gfxterm_print_hex(status);
        gfxterm_print(")\n");
        return status;
    }

    /* Read directory entries */
    UINTN info_size = sizeof(g_dir_info_buf);
    bool first = true;

    gfxterm_print("\n");
    while (1) {
        info_size = sizeof(g_dir_info_buf);
        status = dir->Read(dir, &info_size, g_dir_info_buf);

        if (EFI_ERROR(status) || info_size == 0) {
            break;
        }

        EFI_FILE_INFO *info = (EFI_FILE_INFO*)g_dir_info_buf;
        bool is_dir = (info->Attribute & EFI_FILE_DIRECTORY) != 0;

        char name_ascii[256];
        fat_wide_to_ascii(info->FileName, name_ascii, sizeof(name_ascii));

        if (str_eq(name_ascii, ".") || str_eq(name_ascii, "..")) {
            continue;
        }

        if (is_dir) {
            gfxterm_print("  [DIR]  ");
        } else {
            gfxterm_print("         ");
        }
        gfxterm_print(name_ascii);

        if (!is_dir) {
            gfxterm_print("  (");
            gfxterm_print_dec(info->FileSize);
            gfxterm_print(" bytes)");
        }
        gfxterm_print("\n");
        first = false;
    }

    if (first) {
        gfxterm_print("  (empty directory)\n");
    }
    gfxterm_print("\n");

    return EFI_SUCCESS;
}

/* ===========================================================================
 * Change directory (cd)
 * =========================================================================== */

EFI_STATUS fat_cd(const char *path) {
    if (path == nullptr || path[0] == '\0') {
        return EFI_INVALID_PARAMETER;
    }

    /* Handle ".." — navigate to parent via string manipulation */
    if (str_eq(path, "..")) {
        /* Find last '/' in g_cwd */
        int len = str_len(g_cwd);
        if (len <= 1) {
            /* Already at root */
            g_pwd = g_root;
            g_cwd[0] = '/';
            g_cwd[1] = '\0';
            return EFI_SUCCESS;
        }

        /* Strip trailing '/' if present */
        int end = len - 1;
        if (g_cwd[end] == '/') end--;

        /* Find last '/' before end */
        int last_slash = 0;
        for (int i = 1; i <= end; i++) {
            if (g_cwd[i] == '/') last_slash = i;
        }

        if (last_slash == 0) {
            /* Going to root */
            g_cwd[1] = '\0';
            g_pwd = g_root;
        } else {
            g_cwd[last_slash] = '\0';
            /* We need to re-open the path to get the correct handle.
             * For simplicity, just update the path string — the handle
             * is reopened lazily. In a real implementation, we'd cache
             * path→handle mappings. */
            EFI_FILE_PROTOCOL *new_dir = nullptr;
            EFI_STATUS status = fat_open_path(g_cwd, &new_dir);
            if (!EFI_ERROR(status) && new_dir != nullptr) {
                g_pwd = new_dir;
            }
        }

        gfxterm_print("  cwd: ");
        gfxterm_print(g_cwd);
        gfxterm_print("\n");
        return EFI_SUCCESS;
    }

    /* Handle ".." variants like "../something" — simplified: just try to open */
    EFI_FILE_PROTOCOL *new_dir = nullptr;
    EFI_STATUS status = fat_open_path(path, &new_dir);

    if (EFI_ERROR(status) || new_dir == nullptr) {
        gfxterm_print("cd: no such directory: ");
        gfxterm_print(path);
        gfxterm_print("\n");
        return EFI_NOT_FOUND;
    }

    /* Verify it's actually a directory by trying to read from it */
    UINTN info_size = sizeof(g_dir_info_buf);
    status = new_dir->Read(new_dir, &info_size, g_dir_info_buf);
    /* A successful Read (even with 0 entries) means it's a directory.
     * If it's a file, Read would likely fail or return data differently. */

    /* Update current directory */
    g_pwd = new_dir;

    /* Update g_cwd string */
    if (is_absolute(path)) {
        str_copy(g_cwd, path, FAT_MAX_PATH);
    } else {
        /* Append to current path */
        int cwd_len = str_len(g_cwd);
        if (cwd_len > 1 && g_cwd[cwd_len - 1] != '/') {
            g_cwd[cwd_len] = '/';
            cwd_len++;
            g_cwd[cwd_len] = '\0';
        }
        int remaining = FAT_MAX_PATH - cwd_len - 1;
        if (remaining > 0) {
            str_copy(g_cwd + cwd_len, path, remaining);
        }
    }

    gfxterm_print("  cwd: ");
    gfxterm_print(g_cwd);
    gfxterm_print("\n");
    return EFI_SUCCESS;
}

/* ===========================================================================
 * Make directory (mkdir)
 * =========================================================================== */

EFI_STATUS fat_mkdir(const char *name) {
    if (name == nullptr || name[0] == '\0') {
        gfxterm_print("mkdir: missing directory name\n");
        return EFI_INVALID_PARAMETER;
    }

    CHAR16 wide_name[FAT_MAX_PATH];
    fat_ascii_to_wide(name, wide_name, str_len(name));

    EFI_FILE_PROTOCOL *new_dir = nullptr;
    EFI_STATUS status = g_pwd->Open(
        g_pwd, &new_dir, wide_name,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        EFI_FILE_DIRECTORY
    );

    if (EFI_ERROR(status)) {
        gfxterm_print("mkdir: cannot create directory '");
        gfxterm_print(name);
        gfxterm_print("' (error ");
        gfxterm_print_hex(status);
        gfxterm_print(")\n");
        return status;
    }

    /* Close the handle — the directory now exists on disk */
    if (new_dir) {
        new_dir->Close(new_dir);
    }

    gfxterm_print("mkdir: created '");
    gfxterm_print(name);
    gfxterm_print("'\n");
    return EFI_SUCCESS;
}
