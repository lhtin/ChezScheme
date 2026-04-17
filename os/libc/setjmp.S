# setjmp.S -- ChezSchemeOS freestanding libc: _setjmp/_longjmp for RV64
#
# jmp_buf layout: 25 x 8 bytes = 200 bytes
#   [0]:   ra    (return address)
#   [8]:   sp    (stack pointer)
#   [16]:  s0/fp (frame pointer)
#   [24]:  s1
#   [32]:  s2
#   [40]:  s3
#   [48]:  s4
#   [56]:  s5
#   [64]:  s6
#   [72]:  s7
#   [80]:  s8
#   [88]:  s9
#   [96]:  s10
#   [104]: s11
#   [112]: fs0
#   [120]: fs1
#   [128]: fs2
#   [136]: fs3
#   [144]: fs4
#   [152]: fs5
#   [160]: fs6
#   [168]: fs7
#   [176]: fs8
#   [184]: fs9
#   [192]: fs10
#   [200]: fs11

    .text

# int _setjmp(jmp_buf env)
    .globl _setjmp
    .globl setjmp
    .globl sigsetjmp
    .type _setjmp, @function
    .type setjmp, @function
    .type sigsetjmp, @function
_setjmp:
setjmp:
sigsetjmp:
    # Save callee-saved general-purpose registers
    sd      ra,   0(a0)
    sd      sp,   8(a0)
    sd      s0,  16(a0)
    sd      s1,  24(a0)
    sd      s2,  32(a0)
    sd      s3,  40(a0)
    sd      s4,  48(a0)
    sd      s5,  56(a0)
    sd      s6,  64(a0)
    sd      s7,  72(a0)
    sd      s8,  80(a0)
    sd      s9,  88(a0)
    sd      s10, 96(a0)
    sd      s11,104(a0)

    # Save callee-saved floating-point registers
    fsd     fs0, 112(a0)
    fsd     fs1, 120(a0)
    fsd     fs2, 128(a0)
    fsd     fs3, 136(a0)
    fsd     fs4, 144(a0)
    fsd     fs5, 152(a0)
    fsd     fs6, 160(a0)
    fsd     fs7, 168(a0)
    fsd     fs8, 176(a0)
    fsd     fs9, 184(a0)
    fsd     fs10,192(a0)
    fsd     fs11,200(a0)

    # Return 0
    li      a0, 0
    ret
    .size _setjmp, .-_setjmp

# void _longjmp(jmp_buf env, int val)
    .globl _longjmp
    .globl longjmp
    .globl siglongjmp
    .type _longjmp, @function
    .type longjmp, @function
    .type siglongjmp, @function
_longjmp:
longjmp:
siglongjmp:
    # Restore callee-saved general-purpose registers
    ld      ra,   0(a0)
    ld      sp,   8(a0)
    ld      s0,  16(a0)
    ld      s1,  24(a0)
    ld      s2,  32(a0)
    ld      s3,  40(a0)
    ld      s4,  48(a0)
    ld      s5,  56(a0)
    ld      s6,  64(a0)
    ld      s7,  72(a0)
    ld      s8,  80(a0)
    ld      s9,  88(a0)
    ld      s10, 96(a0)
    ld      s11,104(a0)

    # Restore callee-saved floating-point registers
    fld     fs0, 112(a0)
    fld     fs1, 120(a0)
    fld     fs2, 128(a0)
    fld     fs3, 136(a0)
    fld     fs4, 144(a0)
    fld     fs5, 152(a0)
    fld     fs6, 160(a0)
    fld     fs7, 168(a0)
    fld     fs8, 176(a0)
    fld     fs9, 184(a0)
    fld     fs10,192(a0)
    fld     fs11,200(a0)

    # Return val (or 1 if val == 0)
    mv      a0, a1
    bnez    a0, 1f
    li      a0, 1
1:
    ret
    .size _longjmp, .-_longjmp
