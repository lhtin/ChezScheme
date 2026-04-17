/* setjmp.h -- setjmp/longjmp for bare-metal RV64G */

#ifndef _BAREMETAL_SETJMP_H
#define _BAREMETAL_SETJMP_H

/*
 * RV64 jmp_buf layout (25 registers, 200 bytes):
 *   [0]  ra   (return address)
 *   [1]  sp   (stack pointer)
 *   [2]  s0   (frame pointer)
 *   [3]  s1
 *   [4]  s2
 *   [5]  s3
 *   [6]  s4
 *   [7]  s5
 *   [8]  s6
 *   [9]  s7
 *   [10] s8
 *   [11] s9
 *   [12] s10
 *   [13] s11
 *   [14] fs0  (FP callee-saved)
 *   [15] fs1
 *   [16] fs2
 *   [17] fs3
 *   [18] fs4
 *   [19] fs5
 *   [20] fs6
 *   [21] fs7
 *   [22] fs8
 *   [23] fs9
 *   [24] fs10
 *   [25] fs11
 */
typedef unsigned long jmp_buf[26];

/* sigjmp_buf includes an extra slot for signal mask save flag + mask */
typedef unsigned long sigjmp_buf[26 + 2];

int     setjmp(jmp_buf env);
void    longjmp(jmp_buf env, int val) __attribute__((noreturn));
int     _setjmp(jmp_buf env);
void    _longjmp(jmp_buf env, int val) __attribute__((noreturn));
int     sigsetjmp(sigjmp_buf env, int savesigs);
void    siglongjmp(sigjmp_buf env, int val) __attribute__((noreturn));

#endif /* _BAREMETAL_SETJMP_H */
