/* trap.c -- M-mode trap handler for ChezSchemeOS
 *
 * Dispatches interrupts (timer, external) and exceptions.
 */

#include "uart.h"
#include "timer.h"

void trap_handler(unsigned long mcause, unsigned long mepc, unsigned long mtval) {
    /* Bit 63 of mcause: 1 = interrupt, 0 = exception */
    if (mcause & (1UL << 63)) {
        /* Interrupt */
        unsigned long code = mcause & 0x7F;

        if (code == 7) {
            /* M-mode timer interrupt */
            timer_tick();
            return;  /* resume execution */
        }

        if (code == 11) {
            /* M-mode external interrupt (e.g. UART) — future use */
            return;
        }

        /* Unknown interrupt — print and halt */
        uart_puts("\n*** Unknown interrupt ***\n");
        uart_puts("  mcause = ");
        uart_put_hex(mcause);
        uart_putc('\n');
        while (1) { __asm__ volatile("wfi"); }

    } else {
        /* Exception — print details and halt */
        uart_puts("\n*** TRAP (exception) ***\n");
        uart_puts("  mcause = ");
        uart_put_hex(mcause);
        uart_puts("\n  mepc   = ");
        uart_put_hex(mepc);
        uart_puts("\n  mtval  = ");
        uart_put_hex(mtval);
        uart_putc('\n');
        while (1) { __asm__ volatile("wfi"); }
    }
}
