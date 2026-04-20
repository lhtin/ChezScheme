/* sysinfo.c -- System information primitives for ChezSchemeOS
 *
 * Provides a single foreign function that returns all system info
 * as a Scheme vector of integers. Formatting done in Scheme.
 */

#include "sysinfo.h"
#include "scheme.h"

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

/* Return all sysinfo as a Scheme vector:
 * #(misa mvendorid marchid mimpid mhartid mtime
 *   heap-start heap-end stack-size bss-size code-size
 *   petite-size scheme-size) */
static ptr c_sysinfo_data(void) {
    ptr vec = Smake_vector(13, Sfixnum(0));
    Svector_set(vec, 0, Sunsigned64(read_misa()));
    Svector_set(vec, 1, Sunsigned64(read_mvendorid()));
    Svector_set(vec, 2, Sunsigned64(read_marchid()));
    Svector_set(vec, 3, Sunsigned64(read_mimpid()));
    Svector_set(vec, 4, Sunsigned64(read_mhartid()));
    Svector_set(vec, 5, Sunsigned64(read_rdtime()));
    Svector_set(vec, 6, Sunsigned64((unsigned long)_heap_start));
    Svector_set(vec, 7, Sunsigned64((unsigned long)_heap_end));
    Svector_set(vec, 8, Sunsigned64((unsigned long)(_stack_top - _stack_bottom)));
    Svector_set(vec, 9, Sunsigned64((unsigned long)(_bss_end - _bss_start)));
    Svector_set(vec, 10, Sunsigned64((unsigned long)(_bss_start - (char *)0x80000000UL)));
    Svector_set(vec, 11, Sunsigned64((unsigned long)(_binary_petite_boot_end - _binary_petite_boot_start)));
    Svector_set(vec, 12, Sunsigned64((unsigned long)(_binary_scheme_boot_end - _binary_scheme_boot_start)));
    return vec;
}

/* Register sysinfo foreign symbol */
void sysinfo_register(void) {
    Sforeign_symbol("c_sysinfo_data", (void *)c_sysinfo_data);
}
