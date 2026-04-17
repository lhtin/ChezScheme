# ChezSchemeOS 技术文档

## 概述

ChezSchemeOS 将 Chez Scheme 10.4.0 改造为一个可以直接在 RISC-V 64 位硬件（RV64G）的**机器模式（M-mode）**下运行的操作系统。不依赖任何外部固件（如 OpenSBI）或操作系统（如 Linux），Chez Scheme 的完整 REPL 直接运行在裸机上，通过 QEMU `virt` 机器模拟。

```
+--------------------------------------------------+
|                  QEMU virt (RV64G)                |
|                                                   |
|  +---------------------------------------------+ |
|  |            ChezSchemeOS (M-mode)             | |
|  |                                              | |
|  |  kernel_main.c  ←→  Chez Scheme Runtime      | |
|  |       ↓                    ↓                  | |
|  |   uart.c (I/O)    boot files (petite+scheme)  | |
|  |       ↓                    ↓                  | |
|  |  NS16550A UART     malloc / setjmp / libc     | |
|  +---------------------------------------------+ |
|                                                   |
|  RAM: 0x80000000 - 0x90000000 (256MB)            |
|  UART: 0x10000000 (NS16550A)                     |
+--------------------------------------------------+
```

## 快速开始

```bash
cd os
make clean && make    # 编译
./run.sh              # 在 QEMU 上运行
```

退出 QEMU: `Ctrl-A` 然后 `X`

## 项目结构

```
ChezScheme/
├── c/version.h              ← 修改：添加了 __BAREMETAL_RV64__ 平台块
├── boot/rv64le/             ← 新增：交叉编译的 rv64le 引导文件
│   ├── petite.boot          (2.2MB, Petite Chez Scheme 运行时)
│   ├── scheme.boot          (1.2MB, 完整 Chez Scheme 编译器)
│   ├── scheme.h             (C API 头文件)
│   └── equates.h            (内部常量/偏移量)
│
└── os/                          ← 新增：裸机 OS 全部代码
    ├── boot.S               RV64 汇编入口点
    ├── linker.ld            链接脚本
    ├── kernel_main.c        C 入口 + Chez Scheme 初始化
    ├── uart.h / uart.c      NS16550A UART 驱动
    ├── trap.c               M-mode 异常处理
    ├── Makefile             构建系统
    ├── run.sh               QEMU 启动脚本
    │
    ├── libgcc_override.c   替换 libgcc 中含 C 扩展指令的函数
    │
    ├── chez/                Chez Scheme 平台适配层
    │   ├── config.h         裸机 config（替代构建系统生成的）
    │   └── expeditor_stub.c 表达式编辑器桩函数
    │
    └── libc/                自制 freestanding C 库
        ├── memory.c         malloc / free / realloc
        ├── string.c         memcpy / strlen / strcmp ...
        ├── printf.c         printf / snprintf 系列
        ├── stdio.c          FILE / read / write + 行编辑器
        ├── stdlib.c         abort / exit / qsort / strtol
        ├── setjmp.S         _setjmp / _longjmp (RV64 汇编)
        ├── math.c           sin / cos / exp / log / pow ...
        ├── time.c           clock_gettime (基于 rdtime)
        ├── signal.c         sigaction 桩函数
        ├── ... (共 19 个源文件)
        └── include/         头文件 (共 44 个)
```

## 架构详解

### 1. 启动流程 (`boot.S`)

QEMU `virt` 机器加载 kernel.elf 到 `0x80000000` 并在 M-mode 跳转到 `_start`。

```
_start (boot.S)
  │
  ├── 关闭中断 (csrw mie, zero)
  ├── 启用 FPU (mstatus.FS = 01)    ← RV64G 的 D 扩展需要
  ├── 仅 hart 0 继续，其余 park
  ├── 设置栈指针 → _stack_top
  ├── 清零 BSS 段
  ├── 安装 trap 向量 → trap_entry
  └── call kernel_main()
```

**关键点**：必须在 `mstatus` 中启用 FPU（设置 FS 位），否则任何浮点指令都会触发非法指令异常。这是因为 RV64G 默认 FPU 处于关闭状态。

### 2. 链接脚本 (`linker.ld`)

```
地址空间布局 (256MB RAM):

0x80000000  ┌──────────────┐
            │   .text      │  代码段（含 Chez 运行时）
            ├──────────────┤
            │   .rodata    │  只读数据（含嵌入的 boot 文件 ~3.4MB）
            ├──────────────┤
            │   .data      │  已初始化数据
            ├──────────────┤
            │   .bss       │  未初始化数据
            ├──────────────┤
            │   Stack      │  1MB 栈空间
            ├──────────────┤
            │   Heap       │  剩余空间作为堆（供 malloc 使用）
0x90000000  └──────────────┘
```

### 3. UART 驱动 (`uart.c`)

操作 QEMU `virt` 机器的 NS16550A UART，基地址 `0x10000000`。

| 函数 | 功能 |
|------|------|
| `uart_init()` | 配置 8N1, 使能 FIFO |
| `uart_putc(c)` | 轮询 LSR.THRE 后写 THR |
| `uart_getc()` | 轮询 LSR.DR 后读 RBR（阻塞） |
| `uart_getc_nonblock()` | 非阻塞读取，无数据返回 -1 |
| `uart_puts(s)` | 输出字符串，`\n` 自动转 `\r\n` |
| `uart_put_hex(val)` | 输出 64 位十六进制值 |

### 4. 内核主函数 (`kernel_main.c`)

```c
kernel_main()
  │
  ├── uart_init()                    // 初始化 UART
  ├── Sscheme_init(abnormal_exit)    // 初始化 Chez Scheme 运行时
  ├── Sregister_boot_file_bytes(     // 注册嵌入的 petite.boot
  │     "petite.boot", data, size)
  ├── Sregister_boot_file_bytes(     // 注册嵌入的 scheme.boot
  │     "scheme.boot", data, size)
  ├── Sbuild_heap(NULL, custom_init) // 构建 Scheme 堆
  ├── register_help()                // 注入 (help) 函数
  └── Sscheme_start(0, NULL)         // 启动 REPL → 永不返回
```

**boot 文件嵌入**：通过 `objcopy` 将 `petite.boot` 和 `scheme.boot` 转换为 ELF 目标文件，链接到 `.rodata` 段。这样就不需要文件系统，直接从内存加载。

**`(help)` 函数注入**：在堆构建完成后，通过 Chez 的 C API 调用 `eval(read(open-input-string(...)))` 来定义 Scheme 层面的 `help` 过程。

### 5. Chez Scheme 平台适配 (`c/version.h` 修改)

Chez Scheme 的 `version.h` 通过 `#ifdef` 链来选择操作系统。我们在所有 OS 块之前插入了裸机平台块：

```c
#if defined(__BAREMETAL_RV64__)
  #define USE_MALLOC          // 用 malloc 替代 mmap 分配内存
  #define GETPAGESIZE() 4096  // 固定页面大小
  #define IEEE_DOUBLE         // IEEE 754 双精度浮点
  // ... 其他必要定义 ...
  #undef __linux__            // 阻止 Linux 平台块激活
#endif
```

**为什么用 `USE_MALLOC`**：Chez Scheme 支持三种内存分配策略：`USE_MMAP`（Linux 默认）、`USE_VIRTUAL_ALLOC`（Windows）、`USE_MALLOC`（Emscripten）。裸机没有 mmap，所以使用 malloc 策略，由我们的 `libc/memory.c` 提供实现。

**`#undef __linux__` 的技巧**：交叉编译器 `riscv64-linux-gnu-gcc` 会自动定义 `__linux__`，这会激活 Linux 平台块（包含 mmap、dlopen 等不可用的功能）。通过在裸机块中 `#undef` 掉它，确保只有我们的定义生效。

### 6. 表达式编辑器桩 (`chez/expeditor_stub.c`)

Chez Scheme 的 expeditor（交互式表达式编辑器）依赖 curses 库。裸机上没有 curses，但 boot 文件中包含了 expeditor 代码，它在启动时会通过 `foreign-procedure` 查找 C 函数。如果找不到就会报错退出。

解决方案：注册所有 31 个 expeditor 需要的 C 函数作为桩函数。关键是 `ee_init_term` 返回 `0`（false），告诉 Chez Scheme 终端不可用，使其退回到基本的行 I/O 模式。

```c
void S_expeditor_init(void) {
    Sforeign_symbol("(cs)ee_init_term", (void *)s_ee_init_term);  // 返回 0
    Sforeign_symbol("(cs)ee_read_char", (void *)s_ee_read_char);
    // ... 共 31 个符号
}
```

### 7. Freestanding C 库 (`libc/`)

由于没有操作系统，不能使用 glibc/musl。我们实现了一个最小但完整的 C 库，提供 Chez Scheme C 运行时所需的全部函数。

#### 7.1 内存分配 (`memory.c`)

```
堆布局：
_heap_start → ┌────────────────┐
              │ [hdr][用户数据] │  ← malloc 返回的指针指向用户数据
              │ [hdr][用户数据] │     hdr = { size, magic=0xDEADBEEF }
              │   ...          │
              │ [free list]    │  ← free 后的块加入空闲链表
              │   ...          │
 heap_ptr →   │ (未分配区域)    │  ← bump allocator 的当前位置
              │                │
_heap_end →   └────────────────┘
```

- **分配策略**：先查空闲链表（first-fit），找不到则 bump 分配
- **每个分配 16 字节头**：包含大小和 magic number (0xDEADBEEF) 用于检测越界
- **为什么够用**：Chez Scheme 的 segment allocator 通过大块 malloc 获取内存，然后自己管理分段

#### 7.2 行编辑器 (`stdio.c` 中的 `collect_line`)

我们在 `read()` 系统调用层面实现了一个类 readline 的行编辑器：

| 按键 | 功能 |
|------|------|
| 左/右箭头 | 移动光标 |
| 上/下箭头 | 浏览历史命令（最多 64 条） |
| Backspace | 删除光标前的字符 |
| Delete | 删除光标后的字符 |
| Home / End | 跳到行首/行尾 |
| Ctrl-A / Ctrl-E | 跳到行首/行尾 |
| Ctrl-K | 删除到行尾 |
| Ctrl-U | 清空整行 |
| Ctrl-C | 取消当前输入 |
| Ctrl-D | 空行时发送 EOF |
| Enter (空行) | 重新打印 `>` 提示符 |

**行缓冲设计**：用户输入的字符在按回车前不会传给 Chez Scheme。这确保了表达式在按回车后才开始求值，而不是在右括号匹配时立刻执行。

**历史记录**：使用 64 槽环形缓冲区存储历史命令。浏览历史时保存当前未完成的输入，按下箭头到底可以恢复。

**UTF-8 支持**：行编辑器完整支持多字节 UTF-8 字符（如中文、日文、韩文）：
- 退格键按 UTF-8 字符为单位删除（一个中文字符删除 3 字节，屏幕回退 2 格）
- 左右箭头按完整 UTF-8 字符移动光标，不会拆开多字节序列
- Delete 键删除光标处的完整 UTF-8 字符
- CJK 字符正确识别为 2 个显示宽度，ASCII 为 1 个宽度
- 在行中间插入多字节字符时正确处理重绘

**空行处理**：空行按回车后重新打印 `>` 提示符，与 shell 行为一致。

#### 7.3 setjmp/longjmp (`setjmp.S`)

Chez Scheme 的 continuation 和错误处理严重依赖 setjmp/longjmp。RV64 实现保存/恢复 25 个寄存器：

```
jmp_buf 布局 (200 字节):
  ra, sp, s0-s11 (13 个通用寄存器)
  fs0-fs11       (12 个浮点寄存器)
```

#### 7.4 数学库 (`math.c`)

RV64G 的 D 扩展提供硬件双精度浮点运算。基本运算使用 GCC 内建函数，超越函数使用多项式近似：

| 函数类别 | 实现方式 |
|---------|---------|
| fabs, sqrt, copysign, fmax/fmin | GCC `__builtin_*`（硬件指令） |
| floor, ceil, round, trunc | GCC `__builtin_*` |
| exp | 范围缩减到 [0, ln2] + Taylor 级数 |
| log | frexp 分解 + 级数展开 |
| sin, cos | Chebyshev 多项式 |
| pow | exp(y * log(x)) |
| atan | 分段多项式近似 |
| tgamma | Lanczos 近似 |

#### 7.5 时间 (`time.c`)

使用 RISC-V 的 `rdtime` CSR 指令读取硬件计时器。QEMU `virt` 机器的时钟频率为 10 MHz。

```c
static inline uint64_t rdtime(void) {
    uint64_t val;
    asm volatile("rdtime %0" : "=r"(val));
    return val;
}
// 1 tick = 100ns, clock_gettime 将其转换为 seconds + nanoseconds
```

#### 7.6 桩函数

大量 POSIX 函数以桩形式提供（返回错误码或固定值）：

- **文件系统**：open/stat/opendir 返回 ENOENT（没有文件系统）
- **进程管理**：fork/exec/waitpid 返回 ENOSYS
- **网络**：socket/connect 返回 ENOSYS
- **信号**：sigaction 记录 handler 但不做实际处理
- **动态链接**：dlopen/dlsym 返回 NULL（静态链接）
- **iconv**：ASCII 直通
- **getenv**：始终返回 NULL

### 8. 构建系统 (`Makefile`)

构建过程涉及 5 类目标文件的编译和链接：

```
kernel.elf
  ├── boot.o, uart.o, trap.o, kernel_main.o     (裸机骨架)
  ├── libc/*.o                                    (19 个 freestanding libc)
  ├── chez_objs/scheme.o, alloc.o, ...           (29 个 Chez C 运行时)
  ├── chez_objs/expeditor_stub.o                  (平台桩函数)
  ├── chez_objs/zlib_*.o                          (14 个 zlib 压缩库)
  ├── chez_objs/lz4_*.o                           (4 个 lz4 压缩库)
  └── chez_objs/petite_boot.o, scheme_boot.o      (嵌入的 boot 文件)
```

**两组不同的编译标志**：
- `CFLAGS`：用于裸机代码（boot/kernel/libc），使用 `-isystem libc/include`
- `CHEZ_CFLAGS`：用于 Chez C 运行时，额外定义 `-DRISCV64 -D__BAREMETAL_RV64__ -DSCHEME_STATIC`

**链接**：使用自定义链接脚本 `linker.ld`，链接所有目标文件和 `libgcc.a`（提供软件除法等）。

**纯 RV64G 编译**：所有代码使用 `-march=rv64g`（不含 C 压缩扩展）编译，确保只生成 32 位指令。QEMU 通过 `-cpu rv64,c=false` 禁用 C 扩展。

### 9. Boot 文件交叉编译

rv64le 的 boot 文件通过 Chez Scheme 的构建系统从 pb（portable bytecode）交叉编译而来：

```bash
# 在 ChezScheme 根目录执行：
./configure -m=rv64le --toolprefix=riscv64-linux-gnu- \
  CC_FOR_BUILD=gcc --as-is --disable-curses --disable-x11 --disable-iconv
make rv64le.bootquick
# 输出：boot/rv64le/{petite.boot, scheme.boot, scheme.h, equates.h}
```

`scheme.h` 和 `equates.h` 包含了 rv64le 机器类型的类型定义和内部常量偏移量，是编译 Chez C 运行时所必需的。

### 10. libgcc 覆盖 (`libgcc_override.c`)

系统 `libgcc.a` 由 `riscv64-linux-gnu-gcc` 提供，使用 `rv64gc` 编译（包含 16 位压缩指令）。由于我们的目标是纯 RV64G（无 C 扩展），这些压缩指令会触发非法指令异常。

解决方案：提供以下 libgcc 函数的纯 32 位实现，链接时优先使用我们的版本：

| 函数 | 功能 |
|------|------|
| `__clear_cache` | 指令缓存刷新（使用 `fence.i`） |
| `__clzdi2` | 64 位前导零计数 |
| `__ctzdi2` | 64 位尾部零计数 |
| `__popcountdi2` | 64 位人口计数 |
| `__bswapsi2` / `__bswapdi2` | 32/64 位字节序交换 |
| `__extenddftf2` | double → long double 扩展 |

## 关键设计决策

| 决策 | 原因 |
|------|------|
| 使用 `USE_MALLOC` 而非 `USE_MMAP` | 裸机没有 mmap 系统调用，malloc 是最简单的策略 |
| 非线程构建 (`rv64le` 而非 `trv64le`) | 消除 pthread 依赖，裸机不需要多线程 |
| 嵌入 boot 文件到 ELF | 避免需要文件系统来加载 boot 文件 |
| `#undef __linux__` | 阻止交叉编译器的隐式 Linux 定义激活错误的平台代码 |
| 行缓冲 stdin | 确保按回车后才求值，而非括号匹配后立刻求值 |
| `ee_init_term` 返回 0 | 告诉 Chez Scheme 不使用 expeditor，回退到基本 I/O |
| FPU 初始化在 boot.S | RV64G 的 FPU 默认关闭，不初始化会触发非法指令异常 |
| 纯 RV64G（无 C 扩展） | 确保所有指令为 32 位，简化硬件实现和调试 |
| libgcc 函数覆盖 | 系统 libgcc 含压缩指令，在纯 RV64G 上会触发非法指令 |
| UTF-8 感知行编辑 | 支持中文等多字节字符的输入、删除和光标移动 |

## 数据流：从键盘输入到 Scheme 求值

```
键盘按键
  ↓
QEMU 虚拟 UART (NS16550A @ 0x10000000)
  ↓
uart_getc()                    [uart.c - 轮询 LSR.DR]
  ↓
collect_line()                 [stdio.c - 行编辑器，回显，历史]
  ↓  (按回车后)
line_buf[] → read(fd=0, ...)   [stdio.c - 返回整行给调用者]
  ↓
Chez Scheme 的 bytevector-read  [new-io.c - S_fd_read]
  ↓
Chez reader (read)             [Scheme 层 - 解析 S 表达式]
  ↓
Chez eval                      [Scheme 层 - 求值]
  ↓
Chez printer (write/display)   [Scheme 层 - 格式化输出]
  ↓
write(fd=1, ...)               [stdio.c - 写到 UART]
  ↓
uart_putc()                    [uart.c - 轮询 LSR.THRE 后写 THR]
  ↓
QEMU 虚拟 UART → 终端显示
```

## 统计

| 指标 | 数值 |
|------|------|
| 新增/修改文件 | 73 个 (72 新增 + 1 修改) |
| 裸机核心代码 | 7 个文件 (boot.S, linker.ld, kernel_main.c, uart.c, trap.c, uart.h, libgcc_override.c) |
| Freestanding libc | 19 个源文件 + 44 个头文件 |
| Chez 平台适配 | 2 个文件 (config.h, expeditor_stub.c) |
| Chez C 运行时编译 | 29 个文件 |
| 压缩库编译 | zlib 14 个 + lz4 4 个 |
| 指令集 | 纯 RV64G（无 C 压缩扩展），全部 32 位指令 |
| 内核大小 | ~5.3MB (含 3.4MB 嵌入 boot 文件) |
| QEMU 内存 | 256MB |
| 原始 Chez Scheme 修改 | 仅 `c/version.h` (+40 行) |
