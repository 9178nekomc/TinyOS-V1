/* TinyOS V1 — COM1 Serial Driver Implementation
 *
 * All I/O port access uses inline assembly (x86 out/in instructions).
 * This is safe because we run at ring 0 in UEFI, which does not
 * virtualize legacy I/O ports on QEMU's i440FX + PIIX3 chipset.
 */

#include "serial.h"

/* ---------------------------------------------------------------------------
 * Low-level I/O port helpers — inline assembly on x86_64
 * --------------------------------------------------------------------------- */

static inline void outb(UINT16 port, UINT8 value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline UINT8 inb(UINT16 port) {
    UINT8 value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outw(UINT16 port, UINT16 value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline UINT16 inw(UINT16 port) {
    UINT16 value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
}

/* COM1 base port address */
#define COM1_PORT 0x3F8

/* Register offsets from COM1 base */
#define COM1_DATA       0   /* Data register (DLAB=0) / Divisor LSB (DLAB=1) */
#define COM1_IER        1   /* Interrupt Enable (DLAB=0) / Divisor MSB (DLAB=1) */
#define COM1_FCR        2   /* FIFO Control (write) / Interrupt Ident (read) */
#define COM1_LCR        3   /* Line Control Register */
#define COM1_MCR        4   /* Modem Control Register */
#define COM1_LSR        5   /* Line Status Register */
#define COM1_MSR        6   /* Modem Status Register */
#define COM1_SCR        7   /* Scratch Register */

/* Line Status Register bits */
#define LSR_DR   0x01   /* Data Ready */
#define LSR_THRE 0x20   /* Transmit Holding Register Empty */

/* ===========================================================================
 * Public API
 * =========================================================================== */

void serial_init() {
    /* Step 1: Disable all interrupts */
    outb(COM1_PORT + COM1_IER, 0x00);

    /* Step 2: Set DLAB (bit 7 of LCR) to access divisor registers */
    outb(COM1_PORT + COM1_LCR, 0x80);

    /* Step 3: Set divisor = 1 (115200 baud with 115200 Hz clock) */
    outb(COM1_PORT + COM1_DATA, 0x01);   /* Divisor LSB */
    outb(COM1_PORT + COM1_IER,  0x00);   /* Divisor MSB */

    /* Step 4: Set LCR = 0x03 (8 data bits, no parity, 1 stop bit, DLAB=0) */
    outb(COM1_PORT + COM1_LCR, 0x03);

    /* Step 5: Enable and reset FIFOs (enable, clear RX, clear TX) */
    outb(COM1_PORT + COM1_FCR, 0x07);

    /* Step 6: Set MCR: DTR=1, RTS=1, OUT2=1 (enable IRQs in hardware),
     *         loopback=0 */
    outb(COM1_PORT + COM1_MCR, 0x0B);

    /* Step 7: Optional — clear any garbage in RX buffer */
    inb(COM1_PORT + COM1_DATA);
}

void serial_putc(char c) {
    /* Wait until Transmit Holding Register is empty */
    while (!(inb(COM1_PORT + COM1_LSR) & LSR_THRE)) {
        /* spin */
    }

    /* If we need a carriage return before line feed */
    if (c == '\n') {
        /* Wait again for THR empty */
        while (!(inb(COM1_PORT + COM1_LSR) & LSR_THRE)) {
            /* spin */
        }
        outb(COM1_PORT + COM1_DATA, '\r');
    }

    outb(COM1_PORT + COM1_DATA, (UINT8)c);
}

void serial_print(const char *str) {
    while (*str) {
        serial_putc(*str++);
    }
}

void serial_print_hex(UINT64 val) {
    serial_print("0x");
    if (val == 0) {
        serial_putc('0');
        return;
    }

    /* Find first non-zero nibble */
    int shift = 60; /* 64 - 4 */
    while (shift >= 0 && ((val >> shift) & 0xF) == 0) {
        shift -= 4;
    }

    while (shift >= 0) {
        UINT8 nibble = (val >> shift) & 0xF;
        serial_putc(nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
        shift -= 4;
    }
}

void serial_print_dec(UINT64 val) {
    if (val == 0) {
        serial_putc('0');
        return;
    }

    char buf[21]; /* max 20 digits for UINT64 + null */
    int pos = 20;
    buf[pos--] = '\0';

    while (val > 0) {
        buf[pos--] = '0' + (val % 10);
        val /= 10;
    }

    serial_print(&buf[pos + 1]);
}

BOOLEAN serial_can_read() {
    return (inb(COM1_PORT + COM1_LSR) & LSR_DR) ? 1 : 0;
}

char serial_getc() {
    /* Block until data is available */
    while (!(inb(COM1_PORT + COM1_LSR) & LSR_DR)) {
        /* spin */
    }
    return (char)inb(COM1_PORT + COM1_DATA);
}
