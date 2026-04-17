/* timer.h -- Timer subsystem for ChezSchemeOS
 *
 * Provides M-mode timer interrupts and Scheme-callable timer API:
 *   (set-timer seconds callback)      -> timer-id (one-shot)
 *   (set-timer seconds callback #t)   -> timer-id (repeating)
 *   (cancel-timer id)
 *   (timer-info)
 */

#ifndef TIMER_H
#define TIMER_H

/* QEMU virt CLINT (Core Local Interruptor) addresses */
#define CLINT_BASE     0x2000000UL
#define CLINT_MTIME    (*(volatile unsigned long *)(CLINT_BASE + 0xBFF8))
#define CLINT_MTIMECMP (*(volatile unsigned long *)(CLINT_BASE + 0x4000))
#define TIMER_FREQ     10000000UL  /* QEMU virt: 10 MHz */

#define MAX_TIMERS 16

void timer_init(void);              /* Enable timer interrupt in mie/mstatus */
void timer_tick(void);              /* Called from trap handler on timer IRQ */
void timer_run_pending(void);       /* Run deferred callbacks (call from main loop) */
int  timer_has_pending(void);       /* Check if callbacks are pending */
void timer_info_print(void);        /* Print active timers */
void timer_register_scheme(void);   /* Register Scheme functions */

#endif /* TIMER_H */
