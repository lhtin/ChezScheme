/* uart.h -- NS16550A UART driver for QEMU virt machine */

#ifndef UART_H
#define UART_H

/* QEMU virt machine UART0 base address */
#define UART0_BASE  0x10000000UL

void uart_init(void);
void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char *s);
int  uart_getc_nonblock(void);  /* Returns -1 if no data available */
void uart_put_hex(unsigned long val);

#endif /* UART_H */
