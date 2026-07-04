/* TinyOS V1 — PE/COFF Linker Script for UEFI Applications
 * Produces a valid EFI executable with .text, .data, .bss, and .reloc sections.
 * Subsystem 0x0A = EFI Application.
 */

OUTPUT_FORMAT("pei-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(efi_main)

SECTIONS
{
    /* Base address 0 — UEFI relocates us anywhere */
    . = 0;

    /* Code section: must be 4KB aligned per UEFI spec */
    .text : ALIGN(4096)
    {
        *(.text)
        *(.text.*)
    }

    /* Read-only data mixed in with .text for PE/COFF simplicity */
    .rodata : ALIGN(4096)
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Initialized data */
    .data : ALIGN(4096)
    {
        *(.data)
        *(.data.*)
    }

    /* Base relocations — required by UEFI loader */
    .reloc : ALIGN(4096)
    {
        *(.reloc)
        *(.reloc.*)
    }

    /* Zero-initialized data (not in PE file, allocated at load time) */
    .bss : ALIGN(4096)
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}
