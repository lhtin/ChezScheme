/* stats_wrapper.c -- Override S_unique_id for bare-metal
 *
 * The standard stats.c uses /dev/urandom for UUID generation.
 * On bare-metal there's no /dev/urandom. We compile stats.c
 * normally (it will have the urandom S_unique_id), then provide
 * our own override that uses a simple counter + rdtime.
 *
 * Since this file is linked BEFORE stats.o (via CHEZ_SHIM_OBJS
 * appearing before CHEZ_OBJS in the link order), our S_unique_id
 * takes priority.
 */

#include "system.h"

ptr S_unique_id(void) {
    static U32 counter = 0;
    U32 r[4];

    /* Use rdtime + counter for pseudo-random UUID */
    unsigned long t;
    __asm__ volatile("rdtime %0" : "=r"(t));

    counter++;
    r[0] = (U32)(t);
    r[1] = (U32)(t >> 32) ^ counter;
    r[2] = (U32)(counter * 2654435761U);  /* Knuth multiplicative hash */
    r[3] = (U32)(t ^ (counter << 16));

    /* Set UUIDv4 bits */
    r[1] = (r[1] & (U32)0xFFFF0FFF) | (U32)0x00004000;
    r[2] = (r[2] & (U32)0x3FFFFFFF) | (U32)0x80000000;

    return S_add(S_ash(Sunsigned32(r[0]), Sinteger(8*3*sizeof(U32))),
                 S_add(S_ash(Sunsigned32(r[1]), Sinteger(8*2*sizeof(U32))),
                       S_add(S_ash(Sunsigned32(r[2]), Sinteger(8*sizeof(U32))),
                             Sunsigned32(r[3]))));
}
