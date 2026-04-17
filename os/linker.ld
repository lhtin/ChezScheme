/* linker.ld -- Linker script for ChezSchemeOS on QEMU virt RV64G
 *
 * QEMU virt machine RAM starts at 0x80000000.
 * We place code there and put the heap after BSS + stack.
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

BASE_ADDR = 0x80000000;

SECTIONS
{
    . = BASE_ADDR;

    .text : {
        *(.text.boot)
        *(.text .text.*)
    }

    . = ALIGN(4096);
    .rodata : {
        *(.rodata .rodata.*)
    }

    . = ALIGN(4096);
    .data : {
        *(.data .data.*)
        *(.sdata .sdata.*)
    }

    . = ALIGN(4096);
    .bss : {
        _bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        _bss_end = .;
    }

    . = ALIGN(16);
    _stack_bottom = .;
    . += 0x100000;   /* 1 MB stack */
    _stack_top = .;

    . = ALIGN(4096);
    _heap_start = .;

    /* 256 MB total RAM from BASE_ADDR */
    _heap_end = BASE_ADDR + 0x10000000;

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
        *(.note.*)
    }
}
