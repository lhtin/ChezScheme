/* kernel_main.c -- ChezSchemeOS entry point
 *
 * Initializes hardware, loads embedded Chez Scheme boot files,
 * and starts the Scheme REPL over UART.
 */

#include "uart.h"
#include "sysinfo.h"
#include "timer.h"
#include "scheme.h"
#include <stddef.h>

/* Boot file data embedded via objcopy (from chez_objs/) */
extern unsigned char _binary_petite_boot_start[];
extern unsigned char _binary_petite_boot_end[];
extern unsigned char _binary_scheme_boot_start[];
extern unsigned char _binary_scheme_boot_end[];

static void baremetal_abnormal_exit(void) {
    uart_puts("\n*** Chez Scheme abnormal exit ***\n");
    while (1) { __asm__ volatile("wfi"); }
}

/* Called after Sbuild_heap to register custom Scheme definitions */
static void custom_init(void) {
    /* This is called during Sbuild_heap -- environment is not fully ready yet.
     * We leave it empty; definitions are injected after heap is built. */
}

/* Evaluate a Scheme expression string */
static void eval_scheme_string(const char *code) {
    ptr eval_proc = Stop_level_value(Sstring_to_symbol("eval"));
    ptr read_proc = Stop_level_value(Sstring_to_symbol("read"));
    ptr ois_proc = Stop_level_value(Sstring_to_symbol("open-input-string"));
    ptr port = Scall1(ois_proc, Sstring(code));
    ptr expr = Scall1(read_proc, port);
    Scall1(eval_proc, expr);
}

static void register_help(void) {
    eval_scheme_string(
        "(define (help)"
        "  (display \"\\n\")"
        "  (display \"========================================\\n\")"
        "  (display \"  ChezSchemeOS - Chez Scheme on RV64G\\n\")"
        "  (display \"========================================\\n\")"
        "  (display \"\\n\")"
        "  (display \"This is a full Chez Scheme REPL running\\n\")"
        "  (display \"bare-metal on RISC-V 64-bit (M-mode).\\n\")"
        "  (display \"\\n\")"
        "  (display \"Examples:\\n\")"
        "  (display \"  (+ 1 2)                  => 3\\n\")"
        "  (display \"  (* 6 7)                  => 42\\n\")"
        "  (display \"  (expt 2 64)              => bignum\\n\")"
        "  (display \"  (map car '((a 1) (b 2))) => (a b)\\n\")"
        "  (display \"  (let ((x 10)) (* x x))   => 100\\n\")"
        "  (display \"\\n\")"
        "  (display \"Commands:\\n\")"
        "  (display \"  (help)               show this help\\n\")"
        "  (display \"  (sysinfo)            system information\\n\")"
        "  (display \"  (clear)              clear screen\\n\")"
        "  (display \"  (set-timer s fn)     one-shot timer (seconds)\\n\")"
        "  (display \"  (set-timer s fn #t)  repeating timer\\n\")"
        "  (display \"  (cancel-timer id)    cancel a timer\\n\")"
        "  (display \"  (timer-info)         show active timers\\n\")"
        "  (display \"  (machine-type)       machine type\\n\")"
        "  (display \"  (scheme-version)     version string\\n\")"
        "  (display \"  (collect)            run GC\\n\")"
        "  (display \"\\n\")"
        "  (display \"Exit QEMU: Ctrl-A then X\\n\")"
        "  (display \"\\n\")"
        "  (void))"
    );
}

void kernel_main(void) {
    uart_init();

    uart_puts("\n");
    uart_puts("========================================\n");
    uart_puts("  ChezSchemeOS on RV64G (M-mode)\n");
    uart_puts("  QEMU virt machine\n");
    uart_puts("========================================\n");
    uart_puts("\n");

    uart_puts("Initializing Chez Scheme...\n");
    Sscheme_init(baremetal_abnormal_exit);

    unsigned long petite_size = _binary_petite_boot_end - _binary_petite_boot_start;
    unsigned long scheme_size = _binary_scheme_boot_end - _binary_scheme_boot_start;

    uart_puts("  petite.boot: ");
    uart_put_hex(petite_size);
    uart_puts(" bytes\n");

    uart_puts("  scheme.boot: ");
    uart_put_hex(scheme_size);
    uart_puts(" bytes\n");

    Sregister_boot_file_bytes("petite.boot",
        _binary_petite_boot_start, petite_size);
    Sregister_boot_file_bytes("scheme.boot",
        _binary_scheme_boot_start, scheme_size);

    uart_puts("Building heap...\n");
    Sbuild_heap(NULL, custom_init);

    /* Register custom Scheme functions */
    register_help();

    /* Register (sysinfo) */
    Sforeign_symbol("sysinfo_print", (void *)sysinfo_print);
    eval_scheme_string(
        "(define sysinfo"
        "  (let ((f (foreign-procedure \"sysinfo_print\" () void)))"
        "    (lambda () (f) (void))))"
    );

    /* Register (clear) — clear screen via ANSI escape */
    eval_scheme_string(
        "(define (clear)"
        "  (display \"\\x1b;[2J\\x1b;[H\")"
        "  (void))"
    );

    /* Initialize timer subsystem and register Scheme functions */
    timer_init();
    timer_register_scheme();
    eval_scheme_string(
        "(define set-timer"
        "  (let ((f (foreign-procedure \"scheme_set_timer\" (ptr ptr ptr) ptr)))"
        "    (case-lambda"
        "      ((s cb) (f s cb #f))"
        "      ((s cb repeat) (f s cb repeat)))))"
    );
    eval_scheme_string(
        "(define cancel-timer"
        "  (let ((f (foreign-procedure \"scheme_cancel_timer\" (ptr) ptr)))"
        "    (lambda (id) (f id))))"
    );
    eval_scheme_string(
        "(define timer-info"
        "  (let ((f (foreign-procedure \"scheme_timer_info\" () void)))"
        "    (lambda () (f) (void))))"
    );

    uart_puts("Starting Scheme REPL...\n");
    uart_puts("Type (help) for usage information.\n\n");
    Sscheme_start(0, NULL);

    /* Should not reach here */
    uart_puts("\nScheme exited.\n");
    while (1) { __asm__ volatile("wfi"); }
}
