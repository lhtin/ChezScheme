#!/bin/bash
# run.sh -- Launch ChezSchemeOS on QEMU virt machine (RV64G, M-mode)
#
# -bios none  : No firmware (SBI), boot directly in M-mode at 0x80000000
# -cpu rv64,c=false : RV64G only, disable C (compressed) extension
# -nographic  : UART on terminal
# Exit QEMU: Ctrl-A X

exec qemu-system-riscv64 \
    -machine virt \
    -cpu rv64,c=false \
    -bios none \
    -kernel kernel.elf \
    -nographic \
    -smp 1 \
    -m 256M
