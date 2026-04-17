/* sysinfo.c -- System information for ChezSchemeOS
 *
 * Reads hardware CSRs and linker symbols to display system status.
 * Provides (sysinfo) Scheme function via foreign-procedure.
 */

#include "uart.h"
#include "sysinfo.h"

/* Linker-defined symbols */
extern char _bss_start[], _bss_end[];
extern char _stack_bottom[], _stack_top[];
extern char _heap_start[], _heap_end[];
extern char _binary_petite_boot_start[], _binary_petite_boot_end[];
extern char _binary_scheme_boot_start[], _binary_scheme_boot_end[];

/* Read RISC-V CSRs */
static inline unsigned long read_misa(void) {
    unsigned long val;
    __asm__ volatile("csrr %0, misa" : "=r"(val));
    return val;
}

static inline unsigned long read_mvendorid(void) {
    unsigned long val;
    __asm__ volatile("csrr %0, mvendorid" : "=r"(val));
    return val;
}

static inline unsigned long read_marchid(void) {
    unsigned long val;
    __asm__ volatile("csrr %0, marchid" : "=r"(val));
    return val;
}

static inline unsigned long read_mimpid(void) {
    unsigned long val;
    __asm__ volatile("csrr %0, mimpid" : "=r"(val));
    return val;
}

static inline unsigned long read_mhartid(void) {
    unsigned long val;
    __asm__ volatile("csrr %0, mhartid" : "=r"(val));
    return val;
}

static inline unsigned long read_rdtime(void) {
    unsigned long val;
    __asm__ volatile("rdtime %0" : "=r"(val));
    return val;
}

/* Print a decimal number */
static void print_dec(unsigned long val) {
    char buf[24];
    int i = 0;
    if (val == 0) { uart_putc('0'); return; }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    while (i > 0) uart_putc(buf[--i]);
}

/* Print size in human-readable format */
static void print_size(unsigned long bytes) {
    if (bytes >= 1024UL * 1024 * 1024) {
        print_dec(bytes / (1024UL * 1024 * 1024));
        uart_puts(" GB");
    } else if (bytes >= 1024 * 1024) {
        print_dec(bytes / (1024 * 1024));
        uart_puts(" MB");
    } else if (bytes >= 1024) {
        print_dec(bytes / 1024);
        uart_puts(" KB");
    } else {
        print_dec(bytes);
        uart_puts(" B");
    }
}

void sysinfo_print(void) {
    unsigned long misa = read_misa();
    unsigned long mvendorid = read_mvendorid();
    unsigned long marchid = read_marchid();
    unsigned long mimpid = read_mimpid();
    unsigned long mhartid = read_mhartid();
    unsigned long ticks = read_rdtime();

    uart_puts("\n");
    uart_puts("================== System Info ==================\n");
    uart_puts("\n");

    /* --- CPU --- */
    uart_puts("  CPU\n");

    /* ISA width */
    int mxl = (int)(misa >> 62);  /* top 2 bits: 1=32, 2=64, 3=128 */
    uart_puts("    Architecture:   RV");
    if (mxl == 1) uart_puts("32");
    else if (mxl == 2) uart_puts("64");
    else if (mxl == 3) uart_puts("128");
    else uart_puts("??");

    /* ISA extensions from misa bits [25:0] */
    uart_puts(" (");
    for (int i = 0; i < 26; i++) {
        if (misa & (1UL << i)) {
            uart_putc('A' + i);
        }
    }
    uart_puts(")\n");

    uart_puts("    Vendor ID:      ");
    if (mvendorid == 0)
        uart_puts("0 (not implemented)\n");
    else {
        uart_put_hex(mvendorid);
        uart_putc('\n');
    }

    uart_puts("    Architecture ID: ");
    uart_put_hex(marchid);
    uart_putc('\n');

    uart_puts("    Implementation: ");
    uart_put_hex(mimpid);
    uart_putc('\n');

    uart_puts("    Hart ID:        ");
    print_dec(mhartid);
    uart_putc('\n');

    uart_puts("    Privilege:      M-mode (Machine)\n");

    uart_puts("\n");

    /* --- Memory --- */
    uart_puts("  Memory\n");

    unsigned long total_ram = (unsigned long)(_heap_end - (char *)0x80000000UL);
    unsigned long heap_size = (unsigned long)(_heap_end - _heap_start);
    unsigned long stack_size = (unsigned long)(_stack_top - _stack_bottom);
    unsigned long bss_size = (unsigned long)(_bss_end - _bss_start);
    unsigned long code_size = (unsigned long)(_bss_start - (char *)0x80000000UL);

    uart_puts("    Total RAM:      ");
    print_size(total_ram);
    uart_puts(" (0x80000000 - ");
    uart_put_hex((unsigned long)_heap_end);
    uart_puts(")\n");

    uart_puts("    Code + Data:    ");
    print_size(code_size);
    uart_putc('\n');

    uart_puts("    BSS:            ");
    print_size(bss_size);
    uart_putc('\n');

    uart_puts("    Stack:          ");
    print_size(stack_size);
    uart_putc('\n');

    uart_puts("    Heap:           ");
    print_size(heap_size);
    uart_puts(" (available for malloc)\n");

    uart_puts("\n");

    /* --- Boot files --- */
    uart_puts("  Boot Files\n");

    unsigned long petite_size = (unsigned long)(_binary_petite_boot_end - _binary_petite_boot_start);
    unsigned long scheme_size = (unsigned long)(_binary_scheme_boot_end - _binary_scheme_boot_start);

    uart_puts("    petite.boot:    ");
    print_size(petite_size);
    uart_putc('\n');

    uart_puts("    scheme.boot:    ");
    print_size(scheme_size);
    uart_putc('\n');

    uart_puts("\n");

    /* --- Storage --- */
    uart_puts("  Storage\n");
    uart_puts("    Disk:           none (no filesystem)\n");
    uart_puts("    Boot method:    embedded in kernel ELF\n");

    uart_puts("\n");

    /* --- Uptime --- */
    uart_puts("  Runtime\n");

    unsigned long freq = 10000000UL;  /* QEMU virt: 10 MHz */
    unsigned long secs = ticks / freq;
    unsigned long mins = secs / 60;
    unsigned long hours = mins / 60;

    uart_puts("    Uptime:         ");
    if (hours > 0) { print_dec(hours); uart_puts("h "); }
    if (mins > 0) { print_dec(mins % 60); uart_puts("m "); }
    print_dec(secs % 60);
    uart_puts("s\n");

    uart_puts("    Timer freq:     ");
    print_dec(freq / 1000000);
    uart_puts(" MHz\n");

    uart_puts("\n");

    /* --- Platform --- */
    uart_puts("  Platform\n");
    uart_puts("    Machine:        QEMU virt\n");
    uart_puts("    UART:           NS16550A @ 0x10000000\n");
    uart_puts("    OS:             ChezSchemeOS\n");
    uart_puts("    Scheme:         Chez Scheme 10.4.0\n");
    uart_puts("    Machine type:   rv64le\n");

    uart_puts("\n");
    uart_puts("=================================================\n");
}
