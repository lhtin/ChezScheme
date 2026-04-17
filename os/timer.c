/* timer.c -- Timer subsystem for ChezSchemeOS
 *
 * Uses QEMU virt CLINT timer (mtime/mtimecmp) for M-mode timer interrupts.
 * Callbacks are deferred to main loop context (not called from ISR)
 * to avoid Scheme GC reentrancy issues.
 */

#include "timer.h"
#include "uart.h"
#include "scheme.h"

/* --- Timer table --- */

typedef struct {
    int active;
    int pending;           /* callback deferred, waiting to run */
    int id;
    unsigned long deadline; /* absolute mtime tick */
    unsigned long interval; /* ticks; 0 = one-shot */
    ptr callback;           /* Scheme procedure (GC-locked) */
} timer_entry_t;

static timer_entry_t timers[MAX_TIMERS];
static int next_id = 1;
static volatile int timers_pending = 0;

/* --- CLINT helpers --- */

static inline unsigned long read_mtime(void) {
    return CLINT_MTIME;
}

static void set_mtimecmp(unsigned long val) {
    CLINT_MTIMECMP = val;
}

/* Reprogram mtimecmp to the nearest active timer deadline */
static void reprogram_next(void) {
    unsigned long nearest = (unsigned long)-1;
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].active && !timers[i].pending && timers[i].deadline < nearest) {
            nearest = timers[i].deadline;
        }
    }
    set_mtimecmp(nearest);
}

/* --- Print helpers --- */

static void print_dec(unsigned long val) {
    char buf[24];
    int i = 0;
    if (val == 0) { uart_putc('0'); return; }
    while (val > 0) { buf[i++] = '0' + (val % 10); val /= 10; }
    while (i > 0) uart_putc(buf[--i]);
}

/* --- Core API --- */

void timer_init(void) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        timers[i].active = 0;
        timers[i].pending = 0;
    }
    /* Set mtimecmp to max so no immediate interrupt */
    set_mtimecmp((unsigned long)-1);

    /* Enable M-mode timer interrupt: mie.MTIE = bit 7 */
    unsigned long mie;
    __asm__ volatile("csrr %0, mie" : "=r"(mie));
    mie |= (1UL << 7);
    __asm__ volatile("csrw mie, %0" : : "r"(mie));

    /* Enable global M-mode interrupts: mstatus.MIE = bit 3 */
    unsigned long mstatus;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
    mstatus |= (1UL << 3);
    __asm__ volatile("csrw mstatus, %0" : : "r"(mstatus));
}

/* Add a timer. seconds=duration, callback=Scheme procedure, repeat=interval */
static int timer_add(unsigned long seconds, ptr callback, int repeat) {
    int slot = -1;
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timers[i].active) { slot = i; break; }
    }
    if (slot < 0) return -1; /* no free slot */

    unsigned long ticks = seconds * TIMER_FREQ;
    unsigned long now = read_mtime();

    timers[slot].active = 1;
    timers[slot].pending = 0;
    timers[slot].id = next_id++;
    timers[slot].deadline = now + ticks;
    timers[slot].interval = repeat ? ticks : 0;
    timers[slot].callback = callback;

    /* Protect callback from GC */
    Slock_object(callback);

    reprogram_next();
    return timers[slot].id;
}

static void timer_cancel_by_id(int id) {
    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].active && timers[i].id == id) {
            Sunlock_object(timers[i].callback);
            timers[i].active = 0;
            timers[i].pending = 0;
            reprogram_next();
            return;
        }
    }
}

/* Called from trap handler (ISR context) — must be fast, no Scheme calls */
void timer_tick(void) {
    unsigned long now = read_mtime();

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].active && !timers[i].pending && timers[i].deadline <= now) {
            timers[i].pending = 1;
            timers_pending = 1;
        }
    }

    reprogram_next();
}

/* Check if there are pending callbacks */
int timer_has_pending(void) {
    return timers_pending;
}

/* Run deferred callbacks — called from main loop, safe for Scheme */
void timer_run_pending(void) {
    if (!timers_pending) return;
    timers_pending = 0;

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].active && timers[i].pending) {
            timers[i].pending = 0;

            /* Call the Scheme callback */
            Scall0(timers[i].callback);

            if (timers[i].interval > 0) {
                /* Repeating: schedule next */
                timers[i].deadline = read_mtime() + timers[i].interval;
            } else {
                /* One-shot: deactivate */
                Sunlock_object(timers[i].callback);
                timers[i].active = 0;
            }
        }
    }

    reprogram_next();
}

/* Print active timers */
void timer_info_print(void) {
    unsigned long now = read_mtime();
    int count = 0;

    uart_puts("\n");
    uart_puts("================ Active Timers ================\n");
    uart_puts("\n");

    for (int i = 0; i < MAX_TIMERS; i++) {
        if (!timers[i].active) continue;
        count++;

        uart_puts("  #");
        print_dec(timers[i].id);

        if (timers[i].interval > 0) {
            uart_puts("  repeating  interval: ");
            print_dec(timers[i].interval / TIMER_FREQ);
            uart_puts("s");
        } else {
            uart_puts("  one-shot ");
        }

        if (timers[i].deadline > now) {
            unsigned long remaining = (timers[i].deadline - now) / TIMER_FREQ;
            uart_puts("  remaining: ");
            print_dec(remaining);
            uart_puts("s");
        } else {
            uart_puts("  (pending)");
        }

        uart_puts("  callback: ");
        /* Print Scheme object representation */
        if (Sprocedurep(timers[i].callback)) {
            uart_puts("#<procedure>");
        } else {
            uart_puts("#<object>");
        }
        uart_putc('\n');
    }

    if (count == 0) {
        uart_puts("  (no active timers)\n");
    }

    uart_puts("\n");
    uart_puts("  Slots: ");
    print_dec(count);
    uart_puts("/");
    print_dec(MAX_TIMERS);
    uart_puts(" used\n");

    uart_puts("\n");
    uart_puts("===============================================\n");
}

/* --- Scheme foreign functions --- */

/* (set-timer seconds callback) or (set-timer seconds callback #t) */
static ptr scheme_set_timer(ptr s_seconds, ptr s_callback, ptr s_repeat) {
    unsigned long seconds;
    if (Sfixnump(s_seconds)) {
        seconds = (unsigned long)Sfixnum_value(s_seconds);
    } else {
        /* Handle flonum for fractional seconds */
        seconds = (unsigned long)Sflonum_value(s_seconds);
    }

    int repeat = (s_repeat != Sfalse);
    int id = timer_add(seconds, s_callback, repeat);

    if (id < 0) {
        uart_puts("Error: no free timer slots\n");
        return Sfalse;
    }
    return Sfixnum(id);
}

/* (cancel-timer id) */
static ptr scheme_cancel_timer(ptr s_id) {
    if (!Sfixnump(s_id)) return Sfalse;
    int id = (int)Sfixnum_value(s_id);
    timer_cancel_by_id(id);
    return Svoid;
}

/* (timer-info) */
static void scheme_timer_info(void) {
    timer_info_print();
}

/* Register all timer-related Scheme functions */
void timer_register_scheme(void) {
    Sforeign_symbol("scheme_set_timer", (void *)scheme_set_timer);
    Sforeign_symbol("scheme_cancel_timer", (void *)scheme_cancel_timer);
    Sforeign_symbol("scheme_timer_info", (void *)scheme_timer_info);
}
