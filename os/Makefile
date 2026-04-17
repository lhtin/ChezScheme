# Makefile -- ChezSchemeOS bare-metal build for RV64G / QEMU virt
#
# Usage:
#   make          -- build kernel.elf
#   make run      -- build and run on QEMU
#   make clean    -- remove build artifacts

CHEZ_ROOT = ..
CHEZ_SRC  = $(CHEZ_ROOT)/c
CHEZ_BOOT = boot
ZLIB_DIR  = $(CHEZ_ROOT)/zlib
LZ4_DIR   = $(CHEZ_ROOT)/lz4/lib

CROSS    = riscv64-linux-gnu-
CC       = $(CROSS)gcc
AS       = $(CROSS)gcc
LD       = $(CROSS)ld
OBJCOPY  = $(CROSS)objcopy
OBJDUMP  = $(CROSS)objdump

LIBGCC   = $(shell $(CC) -march=rv64g -mabi=lp64d -print-libgcc-file-name)

COMMON_FLAGS = -march=rv64g -mabi=lp64d -mcmodel=medany \
               -ffreestanding -nostdlib -fno-builtin -O2 -g

CFLAGS   = $(COMMON_FLAGS) -Wall -I. -isystem libc/include -I$(CHEZ_BOOT)

CHEZ_CFLAGS = $(COMMON_FLAGS) \
              -DRISCV64 -D__BAREMETAL_RV64__ -DSCHEME_STATIC \
              -include chez/baremetal_pre.h \
              -Ichez -I$(CHEZ_BOOT) -I$(CHEZ_SRC) -I$(ZLIB_DIR) -I$(LZ4_DIR) \
              -isystem libc/include \
              -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-sign-compare

ASFLAGS  = -march=rv64g -mabi=lp64d -mcmodel=medany -nostdlib -I. -isystem libc/include
LDFLAGS  = -T linker.ld -nostdlib

# --- Boot skeleton ---
BOOT_OBJS = boot.o uart.o trap.o kernel_main.o libgcc_override.o sysinfo.o

# --- Freestanding libc ---
LIBC_C_SRCS = $(wildcard libc/*.c)
LIBC_OBJS   = $(LIBC_C_SRCS:.c=.o) libc/setjmp.o

# --- Chez Scheme C runtime ---
# Note: gc.c is #included by gc-*.c, not compiled standalone
# Note: expeditor.c excluded (no curses), stub provided
CHEZ_C_FILES = scheme alloc segment intern symbol statics \
               gcwrapper gc-011 gc-par gc-ocd gc-oce \
               fasl vfasl compress-io number print \
               prim prim5 schlib schsig io new-io \
               foreign ffi thread random flushcache clearcache self-exe
CHEZ_OBJS = $(addprefix chez_objs/, $(addsuffix .o, $(CHEZ_C_FILES)))

# --- Chez shim/stub objects ---
CHEZ_SHIM_OBJS = chez_objs/expeditor_stub.o chez_objs/stats_wrapper.o

# --- zlib ---
ZLIB_C_FILES = adler32 compress crc32 deflate gzclose gzlib gzread gzwrite \
               infback inffast inflate inftrees trees uncompr zutil
ZLIB_OBJS = $(addprefix chez_objs/zlib_, $(addsuffix .o, $(ZLIB_C_FILES)))

# --- lz4 ---
LZ4_C_FILES = lz4 lz4frame lz4hc xxhash
LZ4_OBJS = $(addprefix chez_objs/lz4_, $(addsuffix .o, $(LZ4_C_FILES)))

# --- Embedded boot files ---
BOOT_DATA_OBJS = chez_objs/petite_boot.o chez_objs/scheme_boot.o

# --- All objects ---
ALL_OBJS = $(BOOT_OBJS) $(LIBC_OBJS) $(CHEZ_OBJS) $(CHEZ_SHIM_OBJS) $(ZLIB_OBJS) $(LZ4_OBJS) $(BOOT_DATA_OBJS)

# === Main target ===
kernel.elf: chez_objs $(ALL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS) $(LIBGCC)
	@echo ""
	@echo "=== Build complete: kernel.elf ==="
	@$(CROSS)size $@

chez_objs:
	mkdir -p chez_objs

# --- Pattern rules for boot/kernel ---
%.o: %.S
	$(AS) $(ASFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# --- Libc ---
libc/%.o: libc/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

libc/%.o: libc/%.S
	$(AS) $(ASFLAGS) -c -o $@ $<

# --- Chez Scheme C runtime ---
chez_objs/%.o: $(CHEZ_SRC)/%.c
	$(CC) $(CHEZ_CFLAGS) -c -o $@ $<

# --- Chez shim/stub ---
chez_objs/%.o: chez/%.c
	$(CC) $(CHEZ_CFLAGS) -c -o $@ $<

# --- zlib ---
chez_objs/zlib_%.o: $(ZLIB_DIR)/%.c
	$(CC) $(CHEZ_CFLAGS) -c -o $@ $<

# --- lz4 ---
chez_objs/lz4_%.o: $(LZ4_DIR)/%.c
	$(CC) $(CHEZ_CFLAGS) -c -o $@ $<

# --- Embed boot files ---
# Copy boot files locally to get clean symbol names
chez_objs/petite.boot: $(CHEZ_BOOT)/petite.boot
	cp $< $@

chez_objs/scheme.boot: $(CHEZ_BOOT)/scheme.boot
	cp $< $@

chez_objs/petite_boot.o: chez_objs/petite.boot
	cd chez_objs && $(OBJCOPY) -I binary -O elf64-littleriscv -B riscv \
	  --rename-section .data=.rodata,alloc,load,readonly,data,contents \
	  petite.boot petite_boot.o

chez_objs/scheme_boot.o: chez_objs/scheme.boot
	cd chez_objs && $(OBJCOPY) -I binary -O elf64-littleriscv -B riscv \
	  --rename-section .data=.rodata,alloc,load,readonly,data,contents \
	  scheme.boot scheme_boot.o

# === Utility targets ===
clean:
	rm -f *.o libc/*.o kernel.elf
	rm -rf chez_objs

run: kernel.elf
	./run.sh

test: kernel.elf
	./tests/run_tests.sh kernel.elf

disasm: kernel.elf
	$(OBJDUMP) -d $< | head -200

size: kernel.elf
	$(CROSS)size $@

.PHONY: clean run test disasm size
