/* TinyOS V1 — Console Input Implementation
 *
 * Provides line-buffered input from serial (primary) and UEFI keyboard
 * (secondary).  Serial is checked FIRST and is non-blocking.
 *
 * CRITICAL: ReadKeyStroke() is a BLOCKING call in the UEFI spec.
 * We must check serial first and only call ReadKeyStroke when
 * we know a key is actually waiting (via WaitForEvent with timeout).
 */

#include "console.h"
#include "serial.h"
#include "alloc.h"

/* Global input buffer — lives in .bss, NOT on stack */
static char  g_input_buf[CONSOLE_MAX_LINE];
static int   g_input_len = 0;

/* ===========================================================================
 * Output — always to serial
 * =========================================================================== */

void console_print(const char *str) {
    serial_print(str);
}

void console_putc(char c) {
    serial_putc(c);
}

/* ===========================================================================
 * Input helpers
 * =========================================================================== */

/* Convert EFI_INPUT_KEY's UnicodeChar to ASCII if possible.
 * Returns 0 for non-ASCII or unrepresentable characters. */
static char key_to_ascii(EFI_INPUT_KEY *key) {
    UINT16 uc = key->UnicodeChar;
    if (uc >= 0x20 && uc <= 0x7E) return (char)uc;
    if (uc == '\r') return '\r';
    if (uc == 0x08) return '\b';
    if (uc == '\t') return '\t';
    if (uc == 0) {
        switch (key->ScanCode) {
            case 0x08: return '\b';
            default:   return 0;
        }
    }
    return 0;
}

/* Try to get a character from serial (non-blocking).
 * Returns 1 and sets *c if a character was available, 0 otherwise. */
static int serial_try_getc(char *c) {
    if (serial_can_read()) {
        *c = serial_getc();
        return 1;
    }
    return 0;
}

/* Try to get a character from UEFI keyboard (non-blocking via CheckEvent).
 * Returns 1 and sets *c if a key was available, 0 otherwise. */
static int uefi_try_getc(char *c) {
    if (!g_ST || !g_ST->ConIn) return 0;

    /* CheckEvent returns IMMEDIATELY:
     *   EFI_SUCCESS   = event is signaled (key waiting)
     *   EFI_NOT_READY = no event pending
     * This is truly non-blocking. */
    EFI_STATUS status = g_BS->CheckEvent(g_ST->ConIn->WaitForKey);

    if (status == EFI_SUCCESS) {
        /* A key is pending — ReadKeyStroke will return immediately */
        EFI_INPUT_KEY key;
        status = g_ST->ConIn->ReadKeyStroke(g_ST->ConIn, &key);
        if (status == EFI_SUCCESS) {
            *c = key_to_ascii(&key);
            return (*c != 0) ? 1 : 0;
        }
    }
    return 0;
}

/* ===========================================================================
 * Public: read a line with editing
 * =========================================================================== */

int console_readline(char *buf, int max_len) {
    g_input_len = 0;
    if (max_len > CONSOLE_MAX_LINE) max_len = CONSOLE_MAX_LINE;

    while (1) {
        char c = 0;
        int got = 0;

        /* 1. Check serial FIRST (non-blocking, primary input source) */
        got = serial_try_getc(&c);

        /* 2. If no serial input, try UEFI keyboard (non-blocking) */
        if (!got) {
            got = uefi_try_getc(&c);
        }

        /* 3. If still no input, yield briefly and retry */
        if (!got || c == 0) {
            if (g_BS) g_BS->Stall(10000); /* 10ms — reduce CPU spin */
            continue;
        }

        /* Handle Enter / CR / LF */
        if (c == '\r' || c == '\n') {
            serial_print("\r\n");
            g_input_buf[g_input_len] = '\0';
            for (int i = 0; i <= g_input_len && i < max_len; i++) {
                buf[i] = g_input_buf[i];
            }
            return g_input_len;
        }

        /* Handle Backspace / DEL */
        if (c == '\b' || c == 0x7F) {
            if (g_input_len > 0) {
                g_input_len--;
                serial_print("\b \b");
            }
            continue;
        }

        /* Handle Ctrl+C / Ctrl+D — treat as empty line */
        if (c == 0x03 || c == 0x04) {
            serial_print("^C\r\n");
            buf[0] = '\0';
            return 0;
        }

        /* Handle printable character */
        if (c >= 0x20 && c <= 0x7E && g_input_len < max_len - 1) {
            g_input_buf[g_input_len++] = c;
            serial_putc(c); /* echo */
        }
    }
}
