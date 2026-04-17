/* libgcc_override.c -- Override libgcc functions that use compressed instructions
 *
 * The system libgcc.a is compiled with rv64gc (including C extension).
 * Since we target rv64g (no C extension), we provide our own implementations
 * of all libgcc functions used by the kernel.
 *
 * Compiled with -march=rv64g so all instructions are 32-bit.
 */

/* __clear_cache: flush instruction cache */
void __clear_cache(void *start, void *end) {
    (void)start;
    (void)end;
    __asm__ volatile("fence.i" ::: "memory");
}

/* __clzdi2: count leading zeros (64-bit) */
int __clzdi2(unsigned long x) {
    if (x == 0) return 64;
    int n = 0;
    if ((x & 0xFFFFFFFF00000000UL) == 0) { n += 32; x <<= 32; }
    if ((x & 0xFFFF000000000000UL) == 0) { n += 16; x <<= 16; }
    if ((x & 0xFF00000000000000UL) == 0) { n += 8;  x <<= 8; }
    if ((x & 0xF000000000000000UL) == 0) { n += 4;  x <<= 4; }
    if ((x & 0xC000000000000000UL) == 0) { n += 2;  x <<= 2; }
    if ((x & 0x8000000000000000UL) == 0) { n += 1; }
    return n;
}

/* __ctzdi2: count trailing zeros (64-bit) */
int __ctzdi2(unsigned long x) {
    if (x == 0) return 64;
    int n = 0;
    if ((x & 0x00000000FFFFFFFFUL) == 0) { n += 32; x >>= 32; }
    if ((x & 0x000000000000FFFFUL) == 0) { n += 16; x >>= 16; }
    if ((x & 0x00000000000000FFUL) == 0) { n += 8;  x >>= 8; }
    if ((x & 0x000000000000000FUL) == 0) { n += 4;  x >>= 4; }
    if ((x & 0x0000000000000003UL) == 0) { n += 2;  x >>= 2; }
    if ((x & 0x0000000000000001UL) == 0) { n += 1; }
    return n;
}

/* __popcountdi2: population count (64-bit) */
int __popcountdi2(unsigned long x) {
    x = x - ((x >> 1) & 0x5555555555555555UL);
    x = (x & 0x3333333333333333UL) + ((x >> 2) & 0x3333333333333333UL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FUL;
    return (int)((x * 0x0101010101010101UL) >> 56);
}

/* __bswapsi2: byte swap 32-bit */
unsigned int __bswapsi2(unsigned int x) {
    return ((x & 0xFF000000U) >> 24) |
           ((x & 0x00FF0000U) >> 8)  |
           ((x & 0x0000FF00U) << 8)  |
           ((x & 0x000000FFU) << 24);
}

/* __bswapdi2: byte swap 64-bit */
unsigned long __bswapdi2(unsigned long x) {
    return ((x & 0xFF00000000000000UL) >> 56) |
           ((x & 0x00FF000000000000UL) >> 40) |
           ((x & 0x0000FF0000000000UL) >> 24) |
           ((x & 0x000000FF00000000UL) >> 8)  |
           ((x & 0x00000000FF000000UL) << 8)  |
           ((x & 0x0000000000FF0000UL) << 24) |
           ((x & 0x000000000000FF00UL) << 40) |
           ((x & 0x00000000000000FFUL) << 56);
}

/* __extenddftf2: extend double to long double (128-bit)
 * On RV64 with D extension, long double is 128-bit IEEE quad.
 * Simple implementation: just widen the double.
 * Note: this is called rarely if at all. */
typedef struct { unsigned long lo; unsigned long hi; } tf_t;

tf_t __extenddftf2(double x) {
    /* For bare-metal, long double operations are rare.
     * Provide a minimal conversion that preserves the double value.
     * The quad-precision format has a 15-bit exponent and 112-bit mantissa. */
    tf_t result;
    union { double d; unsigned long u; } du;
    du.d = x;

    unsigned long sign = du.u >> 63;
    long exp = (long)((du.u >> 52) & 0x7FF) - 1023;
    unsigned long frac = du.u & 0x000FFFFFFFFFFFFFUL;

    if (exp == 1024) {
        /* Inf or NaN */
        result.hi = (sign << 63) | 0x7FFF000000000000UL | (frac ? 0x0000800000000000UL : 0);
        result.lo = 0;
    } else if (exp == -1023 && frac == 0) {
        /* Zero */
        result.hi = sign << 63;
        result.lo = 0;
    } else {
        /* Normal: rebias exponent from 1023 to 16383 */
        unsigned long qexp = (unsigned long)(exp + 16383);
        /* Shift 52-bit fraction to 112-bit position: left shift by 60 */
        result.hi = (sign << 63) | (qexp << 48) | (frac >> 4);
        result.lo = frac << 60;
    }
    return result;
}
