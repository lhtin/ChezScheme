/* kernel_main.c -- ChezSchemeOS entry point
 *
 * Initializes hardware, loads embedded Chez Scheme boot files,
 * registers C primitives, loads init.ss, and starts the REPL.
 *
 * This file should rarely need modification. New features should
 * be implemented in Scheme (os/scheme/init.ss).
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

/* Scheme init source embedded via objcopy */
extern unsigned char _binary_init_ss_start[];
extern unsigned char _binary_init_ss_end[];

static void baremetal_abnormal_exit(void) {
    uart_puts("\n*** Chez Scheme abnormal exit ***\n");
    while (1) { __asm__ volatile("wfi"); }
}

/* Called after Sbuild_heap to register custom Scheme definitions */
static void custom_init(void) {
    /* This is called during Sbuild_heap -- environment is not fully ready yet.
     * We leave it empty; definitions are injected after heap is built. */
}

/* Load an embedded Scheme source file (supports multiple top-level expressions).
 * We use a Scheme-level load loop to avoid GC issues with C-local ptr variables. */
static void load_scheme_source(const unsigned char *data, unsigned long size) {
    /* Build the loader as a Scheme expression and eval it once.
     * This avoids holding Scheme object pointers in C locals across GC points. */
    ptr eval_proc = Stop_level_value(Sstring_to_symbol("eval"));
    ptr read_proc = Stop_level_value(Sstring_to_symbol("read"));
    ptr ois_proc = Stop_level_value(Sstring_to_symbol("open-input-string"));

    /* Create the source string */
    ptr src = Sstring_utf8((const char *)data, (iptr)size);

    /* Build and eval: (let ((p (open-input-string src)))
     *                   (let loop ((e (read p)))
     *                     (unless (eof-object? e)
     *                       (eval e)
     *                       (loop (read p))))) */
    ptr load_expr = Scall1(read_proc,
        Scall1(ois_proc,
            Sstring("(lambda (s)"
                    "  (let ((p (open-input-string s)))"
                    "    (let loop ((e (read p)))"
                    "      (unless (eof-object? e)"
                    "        (eval e)"
                    "        (loop (read p))))))")));
    ptr loader = Scall1(eval_proc, load_expr);
    Scall1(loader, src);
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

    /* Register all C primitives for Scheme */
    sysinfo_register();          /* CSR reads, memory layout */
    timer_init();                /* Hardware timer setup */
    timer_register_scheme();     /* Timer foreign functions */

    /* REPL support primitives */
    Sforeign_symbol("stdio_set_prompt", (void *)stdio_set_prompt);
    Sforeign_symbol("timer_set_current_repl", (void *)timer_set_current_repl);
    Sforeign_symbol("timer_flush_repl_buffer", (void *)timer_flush_repl_buffer);

    /* Load Scheme initialization code */
    uart_puts("Loading init.ss...\n");
    load_scheme_source(_binary_init_ss_start,
        _binary_init_ss_end - _binary_init_ss_start);

    uart_puts("Starting Scheme REPL...\n");
    uart_puts("Type (help) for usage information.\n\n");
    Sscheme_start(0, NULL);

    /* Should not reach here */
    uart_puts("\nScheme exited.\n");
    while (1) { __asm__ volatile("wfi"); }
}
