/* uart.c -- NS16550A UART driver for QEMU virt machine */

#include "uart.h"
#include "timer.h"

/* NS16550A register offsets */
#define UART_RBR    0   /* Receive Buffer Register (read) */
#define UART_THR    0   /* Transmit Holding Register (write) */
#define UART_IER    1   /* Interrupt Enable Register */
#define UART_FCR    2   /* FIFO Control Register (write) */
#define UART_LCR    3   /* Line Control Register */
#define UART_MCR    4   /* Modem Control Register */
#define UART_LSR    5   /* Line Status Register */
#define UART_DLL    0   /* Divisor Latch Low (when DLAB=1) */
#define UART_DLH    1   /* Divisor Latch High (when DLAB=1) */

/* LSR bits */
#define LSR_DR      0x01    /* Data Ready */
#define LSR_THRE    0x20    /* Transmit Holding Register Empty */

/* LCR bits */
#define LCR_8BIT    0x03    /* 8-bit data */
#define LCR_DLAB    0x80    /* Divisor Latch Access Bit */

/* FCR bits */
#define FCR_ENABLE  0x01    /* Enable FIFOs */
#define FCR_CLEAR   0x06    /* Clear both FIFOs */

static volatile unsigned char *const uart = (unsigned char *)UART0_BASE;

static inline void uart_write_reg(int reg, unsigned char val) {
    uart[reg] = val;
}

static inline unsigned char uart_read_reg(int reg) {
    return uart[reg];
}

void uart_init(void) {
    /* Disable interrupts */
    uart_write_reg(UART_IER, 0x00);

    /* Set baud rate: enable DLAB */
    uart_write_reg(UART_LCR, LCR_DLAB);

    /* Set divisor to 1 (QEMU doesn't care about actual baud rate) */
    uart_write_reg(UART_DLL, 0x01);
    uart_write_reg(UART_DLH, 0x00);

    /* 8 data bits, 1 stop bit, no parity -- clear DLAB */
    uart_write_reg(UART_LCR, LCR_8BIT);

    /* Enable and clear FIFOs */
    uart_write_reg(UART_FCR, FCR_ENABLE | FCR_CLEAR);

    /* No modem control */
    uart_write_reg(UART_MCR, 0x00);

    /* Enable receive interrupts (for future use) */
    uart_write_reg(UART_IER, 0x01);
}

void uart_putc(char c) {
    /* Wait until transmit holding register is empty */
    while ((uart_read_reg(UART_LSR) & LSR_THRE) == 0)
        ;
    uart_write_reg(UART_THR, c);
}

char uart_getc(void) {
    /* Wait until data is ready, run timer callbacks while waiting.
     * This path is used by Scheme I/O (not the line editor).
     * The line editor in stdio.c uses uart_getc_nonblock() instead. */
    while ((uart_read_reg(UART_LSR) & LSR_DR) == 0) {
        if (timer_has_pending()) timer_run_pending();
    }
    return (char)uart_read_reg(UART_RBR);
}

int uart_getc_nonblock(void) {
    if (uart_read_reg(UART_LSR) & LSR_DR)
        return (int)uart_read_reg(UART_RBR);
    return -1;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s);
        s++;
    }
}

/* Print a 64-bit value in hex */
void uart_put_hex(unsigned long val) {
    static const char hex[] = "0123456789abcdef";
    uart_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uart_putc(hex[(val >> i) & 0xf]);
    }
}
