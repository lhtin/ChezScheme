# boot.S -- RV64G bare-metal M-mode entry point for ChezSchemeOS
#
# QEMU virt machine loads kernel at 0x80000000 and jumps there in M-mode.
# This code sets up the stack, clears BSS, installs trap vector, and
# calls kernel_main().

    .section .text.boot
    .globl _start
_start:
    # Disable interrupts
    csrw    mie, zero
    csrw    mstatus, zero

    # Enable FPU: set mstatus.FS = 01 (Initial)
    # FS field is bits [14:13] of mstatus
    li      t0, (1 << 13)
    csrs    mstatus, t0

    # Set up stack pointer (only hart 0 proceeds, others park)
    csrr    t0, mhartid
    bnez    t0, .park

    # Set stack pointer to top of stack (defined in linker script)
    la      sp, _stack_top

    # Clear BSS section
    la      t0, _bss_start
    la      t1, _bss_end
.bss_clear:
    bgeu    t0, t1, .bss_done
    sd      zero, 0(t0)
    addi    t0, t0, 8
    j       .bss_clear
.bss_done:

    # Set up M-mode trap vector
    la      t0, trap_entry
    csrw    mtvec, t0

    # Call kernel_main -- should never return
    call    kernel_main

    # If it does return, park
.park:
    wfi
    j       .park

# -------------------------------------------------------------------
# M-mode trap entry point
# Saves all registers, calls trap_handler(mcause, mepc, mtval), restores
# -------------------------------------------------------------------
    .align 4
    .globl trap_entry
trap_entry:
    # Allocate stack frame for 32 GP registers (32 * 8 = 256 bytes)
    addi    sp, sp, -256

    # Save all general-purpose registers
    sd      ra,   0(sp)
    sd      t0,   8(sp)
    sd      t1,  16(sp)
    sd      t2,  24(sp)
    sd      s0,  32(sp)
    sd      s1,  40(sp)
    sd      a0,  48(sp)
    sd      a1,  56(sp)
    sd      a2,  64(sp)
    sd      a3,  72(sp)
    sd      a4,  80(sp)
    sd      a5,  88(sp)
    sd      a6,  96(sp)
    sd      a7, 104(sp)
    sd      s2, 112(sp)
    sd      s3, 120(sp)
    sd      s4, 128(sp)
    sd      s5, 136(sp)
    sd      s6, 144(sp)
    sd      s7, 152(sp)
    sd      s8, 160(sp)
    sd      s9, 168(sp)
    sd      s10,176(sp)
    sd      s11,184(sp)
    sd      t3, 192(sp)
    sd      t4, 200(sp)
    sd      t5, 208(sp)
    sd      t6, 216(sp)
    sd      gp, 224(sp)
    sd      tp, 232(sp)

    # Call trap_handler(mcause, mepc, mtval)
    csrr    a0, mcause
    csrr    a1, mepc
    csrr    a2, mtval
    call    trap_handler

    # Restore all general-purpose registers
    ld      ra,   0(sp)
    ld      t0,   8(sp)
    ld      t1,  16(sp)
    ld      t2,  24(sp)
    ld      s0,  32(sp)
    ld      s1,  40(sp)
    ld      a0,  48(sp)
    ld      a1,  56(sp)
    ld      a2,  64(sp)
    ld      a3,  72(sp)
    ld      a4,  80(sp)
    ld      a5,  88(sp)
    ld      a6,  96(sp)
    ld      a7, 104(sp)
    ld      s2, 112(sp)
    ld      s3, 120(sp)
    ld      s4, 128(sp)
    ld      s5, 136(sp)
    ld      s6, 144(sp)
    ld      s7, 152(sp)
    ld      s8, 160(sp)
    ld      s9, 168(sp)
    ld      s10,176(sp)
    ld      s11,184(sp)
    ld      t3, 192(sp)
    ld      t4, 200(sp)
    ld      t5, 208(sp)
    ld      t6, 216(sp)
    ld      gp, 224(sp)
    ld      tp, 232(sp)

    addi    sp, sp, 256

    mret
