/* uuid/uuid.h -- stub for bare-metal (UUID not available) */
#ifndef _BAREMETAL_UUID_H
#define _BAREMETAL_UUID_H

typedef unsigned char uuid_t[16];

static inline void uuid_generate(uuid_t out) {
    /* Simple counter-based fake UUID */
    static unsigned long counter = 0;
    counter++;
    for (int i = 0; i < 16; i++)
        out[i] = (unsigned char)((counter >> (i * 4)) & 0xff);
}

#endif /* _BAREMETAL_UUID_H */
