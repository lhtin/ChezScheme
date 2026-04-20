/* kernel_main.c -- ChezSchemeOS entry point
 *
 * Initializes hardware, loads embedded Chez Scheme boot files,
 * loads init.ss for Scheme-level initialization, and starts the REPL.
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

/* Load an embedded Scheme source file (supports multiple top-level expressions) */
static void load_scheme_source(const unsigned char *data, unsigned long size) {
    ptr ois_proc = Stop_level_value(Sstring_to_symbol("open-input-string"));
    ptr read_proc = Stop_level_value(Sstring_to_symbol("read"));
    ptr eval_proc = Stop_level_value(Sstring_to_symbol("eval"));
    ptr eof_proc = Stop_level_value(Sstring_to_symbol("eof-object?"));

    ptr str = Sstring_utf8((const char *)data, (iptr)size);
    ptr port = Scall1(ois_proc, str);

    while (1) {
        ptr expr = Scall1(read_proc, port);
        if (Scall1(eof_proc, expr) != Sfalse) break;
        Scall1(eval_proc, expr);
    }
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

    /* Register C foreign symbols (required before loading init.ss) */
    Sforeign_symbol("sysinfo_print", (void *)sysinfo_print);
    Sforeign_symbol("stdio_set_prompt", (void *)stdio_set_prompt);
    Sforeign_symbol("timer_set_current_repl", (void *)timer_set_current_repl);
    Sforeign_symbol("timer_flush_repl_buffer", (void *)timer_flush_repl_buffer);

    /* Initialize timer hardware and register timer foreign symbols */
    timer_init();
    timer_register_scheme();

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
