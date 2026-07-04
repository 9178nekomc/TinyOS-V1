/* TinyOS V1 — Memory Allocator
 *
 * Thin wrappers around UEFI Boot Services memory functions.
 * All allocations > 1KB MUST go through os_alloc() to avoid
 * overflowing the UEFI stack (typically only a few KB).
 *
 * Global UEFI state pointers are declared here as they are
 * needed virtually everywhere.
 */

#ifndef TINYOS_ALLOC_H
#define TINYOS_ALLOC_H

#include "uefi/types.h"

/* Global UEFI state — set once in efi_main before any use */
extern EFI_SYSTEM_TABLE  *g_ST;
extern EFI_BOOT_SERVICES *g_BS;
extern EFI_HANDLE         g_IH;

/* Allocate memory from UEFI pool. Returns nullptr on failure.
 * All allocations use EfiLoaderData memory type. */
void *os_alloc(UINTN size);

/* Free memory previously allocated with os_alloc(). */
void os_free(void *ptr);

/* Zero-initialize a block of memory. */
void os_memzero(void *ptr, UINTN size);

#endif /* TINYOS_ALLOC_H */
