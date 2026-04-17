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

/* From libc/stdio.c */
extern void stdio_set_prompt(const char *prompt);

/* From timer.c */
extern void timer_set_current_repl(int id);
extern void timer_flush_repl_buffer(void);

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
        "  (display \"  (new-repl)           create new REPL\\n\")"
        "  (display \"  (repl-list)          list all REPLs\\n\")"
        "  (display \"  (switch-repl n)      switch to REPL #n\\n\")"
        "  (display \"  (close-repl)         close current REPL\\n\")"
        "  (display \"  (machine-type)       machine type\\n\")"
        "  (display \"  (scheme-version)     version string\\n\")"
        "  (display \"  (collect)            run GC\\n\")"
        "  (display \"\\n\")"
        "  (display \"Shortcuts: Ctrl-N next REPL, Ctrl-P prev REPL\\n\")"
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

    /* Multi-REPL system */
    Sforeign_symbol("stdio_set_prompt", (void *)stdio_set_prompt);
    Sforeign_symbol("timer_set_current_repl", (void *)timer_set_current_repl);
    Sforeign_symbol("timer_flush_repl_buffer", (void *)timer_flush_repl_buffer);
    eval_scheme_string(
        "(begin"
        "  (define *repls* (list (interaction-environment)))"
        "  (define *repl-id* 0)"
        ""
        "  (define c-set-prompt (foreign-procedure \"stdio_set_prompt\" (string) void))"
        "  (define c-set-repl-id (foreign-procedure \"timer_set_current_repl\" (int) void))"
        "  (define c-flush-repl-buf (foreign-procedure \"timer_flush_repl_buffer\" () void))"
        ""
        "  (define (update-prompt)"
        "    (let ((p (string-append \"[\" (number->string *repl-id*) \"]> \")))"
        "      (c-set-prompt p)"
        "      (c-set-repl-id *repl-id*)"
        "      (c-flush-repl-buf)"
        "      (waiter-prompt-string p)))"
        ""
        "  (define (new-repl)"
        "    (let ((env (copy-environment (interaction-environment) #t)))"
        "      (set! *repls* (append *repls* (list env)))"
        "      (set! *repl-id* (- (length *repls*) 1))"
        "      (interaction-environment env)"
        "      (update-prompt)"
        "      (display (string-append \"Created REPL #\""
        "        (number->string *repl-id*)"
        "        \" (\" (number->string (length *repls*)) \" total)\\n\"))"
        "      (void)))"
        ""
        "  (define (next-repl)"
        "    (if (<= (length *repls*) 1) (void)"
        "      (begin"
        "        (set! *repl-id* (modulo (+ *repl-id* 1) (length *repls*)))"
        "        (interaction-environment (list-ref *repls* *repl-id*))"
        "        (update-prompt)"
        "        (display (string-append \"Switched to REPL #\""
        "          (number->string *repl-id*)"
        "          \"/\" (number->string (length *repls*)) \"\\n\"))"
        "        (void))))"
        ""
        "  (define (prev-repl)"
        "    (if (<= (length *repls*) 1) (void)"
        "      (begin"
        "        (set! *repl-id* (modulo (- *repl-id* 1) (length *repls*)))"
        "        (interaction-environment (list-ref *repls* *repl-id*))"
        "        (update-prompt)"
        "        (display (string-append \"Switched to REPL #\""
        "          (number->string *repl-id*)"
        "          \"/\" (number->string (length *repls*)) \"\\n\"))"
        "        (void))))"
        ""
        "  (define (switch-repl n)"
        "    (if (and (>= n 0) (< n (length *repls*)))"
        "      (begin"
        "        (set! *repl-id* n)"
        "        (interaction-environment (list-ref *repls* *repl-id*))"
        "        (update-prompt)"
        "        (display (string-append \"Switched to REPL #\""
        "          (number->string *repl-id*) \"\\n\"))"
        "        (void))"
        "      (display (string-append \"Invalid: use 0-\""
        "        (number->string (- (length *repls*) 1)) \"\\n\"))))"
        ""
        "  (define (repl-list)"
        "    (display \"\\nActive REPLs:\\n\")"
        "    (let loop ((i 0) (rest *repls*))"
        "      (when (pair? rest)"
        "        (display (string-append"
        "          (if (= i *repl-id*) \"  * [\" \"    [\")"
        "          (number->string i) \"]\\n\"))"
        "        (loop (+ i 1) (cdr rest))))"
        "    (display (string-append \"  (\" (number->string (length *repls*))"
        "      \" total, * = current)\\n\"))"
        "    (void))"
        ""
        "  (define (close-repl)"
        "    (if (<= (length *repls*) 1)"
        "      (display \"Cannot close the last REPL\\n\")"
        "      (begin"
        "        (set! *repls* (append"
        "          (list-head *repls* *repl-id*)"
        "          (list-tail *repls* (+ *repl-id* 1))))"
        "        (set! *repl-id* (min *repl-id* (- (length *repls*) 1)))"
        "        (interaction-environment (list-ref *repls* *repl-id*))"
        "        (update-prompt)"
        "        (display (string-append \"Closed. Now on REPL #\""
        "          (number->string *repl-id*) \"/\""
        "          (number->string (length *repls*)) \"\\n\"))"
        "        (void))))"
        ")"
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

    /* Set initial prompt to [0]> */
    eval_scheme_string("(waiter-prompt-string \"[0]> \")");

    uart_puts("Starting Scheme REPL...\n");
    uart_puts("Type (help) for usage information.\n\n");
    Sscheme_start(0, NULL);

    /* Should not reach here */
    uart_puts("\nScheme exited.\n");
    while (1) { __asm__ volatile("wfi"); }
}
