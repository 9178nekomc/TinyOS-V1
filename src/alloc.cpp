/* TinyOS V1 — Memory Allocator Implementation */

#include "alloc.h"
#include "serial.h"

/* Global UEFI state — initialized in efi_main */
EFI_SYSTEM_TABLE  *g_ST = nullptr;
EFI_BOOT_SERVICES *g_BS = nullptr;
EFI_HANDLE         g_IH = nullptr;

void *os_alloc(UINTN size) {
    VOID *ptr = nullptr;
    EFI_STATUS status = g_BS->AllocatePool(EfiLoaderData, size, &ptr);
    if (EFI_ERROR(status) || ptr == nullptr) {
        serial_print("[FATAL] os_alloc(");
        serial_print_hex(size);
        serial_print(") failed, status=");
        serial_print_hex(status);
        serial_print("\r\n");
        /* Hang — we cannot recover from OOM at this level */
        for (;;) { __asm__ volatile("hlt"); }
    }
    return ptr;
}

void os_free(void *ptr) {
    if (ptr) {
        g_BS->FreePool(ptr);
    }
}

void os_memzero(void *ptr, UINTN size) {
    UINT8 *p = (UINT8*)ptr;
    for (UINTN i = 0; i < size; i++) {
        p[i] = 0;
    }
}
