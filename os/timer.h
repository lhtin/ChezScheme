/* timer.h -- Timer subsystem for ChezSchemeOS */

#ifndef TIMER_H
#define TIMER_H

/* QEMU virt CLINT addresses */
#define CLINT_BASE     0x2000000UL
#define CLINT_MTIME    (*(volatile unsigned long *)(CLINT_BASE + 0xBFF8))
#define CLINT_MTIMECMP (*(volatile unsigned long *)(CLINT_BASE + 0x4000))
#define TIMER_FREQ     10000000UL

#define MAX_TIMERS 16

void timer_init(void);
void timer_tick(void);              /* Called from ISR */
void timer_run_pending(void);       /* Run deferred callbacks */
int  timer_has_pending(void);
void timer_info_print(void);
void timer_register_scheme(void);

/* Output buffering for multi-REPL */
int  timer_buffer_char(char c);     /* Buffer a char if redirecting */
int  timer_is_buffering(void);      /* Check if currently buffering */
void timer_set_current_repl(int id);/* Set active REPL ID */
void timer_flush_repl_buffer(void); /* Flush buffered output for current REPL */

#endif
