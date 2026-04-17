/* trap.c -- M-mode trap handler for ChezSchemeOS */

#include "uart.h"

void trap_handler(unsigned long mcause, unsigned long mepc, unsigned long mtval) {
    uart_puts("\n*** TRAP ***\n");
    uart_puts("  mcause = ");
    uart_put_hex(mcause);
    uart_puts("\n  mepc   = ");
    uart_put_hex(mepc);
    uart_puts("\n  mtval  = ");
    uart_put_hex(mtval);
    uart_putc('\n');

    /* Halt */
    while (1) {
        __asm__ volatile("wfi");
    }
}
