# ChezSchemeOS 技术文档

## 概述

ChezSchemeOS 将 Chez Scheme 10.4.0 改造为一个可以直接在 RISC-V 64 位硬件（RV64G）的**机器模式（M-mode）**下运行的操作系统。不依赖任何外部固件（如 OpenSBI）或操作系统（如 Linux），Chez Scheme 的完整 REPL 直接运行在裸机上，通过 QEMU `virt` 机器模拟。

**所有改动完全自包含在 `os/` 目录下，原始 Chez Scheme 源码零修改。**

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
make test             # 运行自动化测试 (30 个用例)
```

退出 QEMU: `Ctrl-A` 然后 `X`

## 项目结构

```
ChezScheme/                        ← 原始 Chez Scheme 源码（不修改）
├── c/                             原始 C 运行时源码
├── s/                             原始 Scheme 编译器源码
├── zlib/ lz4/                     压缩库
│
└── os/                            ← 裸机 OS（所有改动在此）
    ├── Makefile                   构建系统
    ├── run.sh                     QEMU 启动脚本
    ├── ARCHITECTURE.md            技术文档
    │
    ├── boot.S                     RV64 汇编入口点（M-mode）
    ├── linker.ld                  链接脚本（内存布局）
    ├── kernel_main.c              C 入口 + 系统信息 + 定时器 + Scheme 初始化
    ├── sysinfo.c                  硬件/内存信息收集（misa, heap, stack 等）
    ├── timer.c                    软件定时器（mtime 轮询，多槽位）
    ├── uart.h / uart.c            NS16550A UART 驱动
    ├── trap.c                     M-mode 异常处理
    ├── libgcc_override.c          替换含 C 扩展指令的 libgcc 函数
    │
    ├── scheme/                    Scheme 源码（嵌入到 ELF）
    │   └── init.ss                启动初始化脚本（sysinfo/timer 封装）
    │
    ├── boot/                      交叉编译的 rv64le 引导文件
    │   ├── petite.boot            (2.2MB, Petite Chez Scheme)
    │   ├── scheme.boot            (1.2MB, 完整编译器)
    │   ├── scheme.h               (C API 头文件)
    │   ├── equates.h              (内部常量/偏移量)
    │   └── gc-*.inc, heapcheck.inc
    │
    ├── chez/                      Chez Scheme 平台适配层
    │   ├── baremetal_pre.h        通过 -include 注入的平台定义
    │   ├── config.h               裸机 config（替代构建系统生成的）
    │   ├── expeditor_stub.c       表达式编辑器桩函数（31 个符号）
    │   └── stats_wrapper.c        stats.c 包装（禁用 /dev/urandom）
    │
    ├── libc/                      自制 freestanding C 库
    │   ├── memory.c               malloc / free / realloc / calloc
    │   ├── string.c               memcpy / strlen / strcmp ...
    │   ├── printf.c               printf / snprintf 系列
    │   ├── stdio.c                FILE / read / write + UTF-8 行编辑器
    │   ├── stdlib.c               abort / exit / qsort / strtol
    │   ├── setjmp.S               _setjmp / _longjmp (RV64 汇编)
    │   ├── math.c                 sin / cos / exp / log / pow ...
    │   ├── time.c                 clock_gettime (基于 rdtime)
    │   ├── signal.c               sigaction 桩函数
    │   ├── unistd.c               POSIX 桩 (fork/exec/getpid/...)
    │   ├── fcntl.c                文件描述符控制桩
    │   ├── ... (共 19 个源文件)
    │   └── include/               头文件 (共 44 个)
    │       ├── stdio.h, stdlib.h, string.h, ...
    │       ├── sys/types.h, sys/stat.h, ...
    │       └── uuid/uuid.h        UUID 桩
    │
    └── tests/
        └── run_tests.sh           自动化测试（30 个用例，8 组）
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
  ├── sysinfo_register()             // 注册硬件/内存信息查询原语
  ├── timer_init()                   // 初始化软件定时器子系统
  ├── Sscheme_init(abnormal_exit)    // 初始化 Chez Scheme 运行时
  ├── Sregister_boot_file_bytes(     // 注册嵌入的 petite.boot
  │     "petite.boot", data, size)
  ├── Sregister_boot_file_bytes(     // 注册嵌入的 scheme.boot
  │     "scheme.boot", data, size)
  ├── Sbuild_heap(NULL, custom_init) // 构建 Scheme 堆
  │     └── custom_init:
  │           ├── register_foreign_procedures()  // 注册 C 原语
  │           └── load_scheme_source("init.ss")  // 加载 Scheme 初始化脚本
  ├── register_help()                // 注入 (help) 函数
  └── Sscheme_start(0, NULL)         // 启动 REPL → 永不返回
```

**boot 文件嵌入**：通过 `objcopy` 将 `petite.boot` 和 `scheme.boot` 转换为 ELF 目标文件，链接到 `.rodata` 段。这样就不需要文件系统，直接从内存加载。

**Scheme 源码加载**：`init.ss` 同样通过 `objcopy` 嵌入 ELF，在堆构建阶段通过 `load_scheme_source` 以 GC 安全的方式加载——使用 `Slock_object` 保护字符串，调用 `eval(read(open-input-string(...)))`，再 `Sunlock_object` 释放。

**`(help)` 函数注入**：在堆构建完成后，通过 Chez 的 C API 调用 `eval(read(open-input-string(...)))` 来定义 Scheme 层面的 `help` 过程。

### 5. 平台适配（零修改原始源码）

我们**不修改** Chez Scheme 的任何原始文件。平台适配完全通过编译参数和注入头文件实现：

#### `chez/baremetal_pre.h`（通过 `-include` 注入）

编译 Chez C 运行时时，通过 GCC 的 `-include chez/baremetal_pre.h` 参数，在所有源文件之前强制包含此头文件。它的作用：

1. **`#undef __linux__`** 等 OS 宏 — 阻止 `c/version.h` 中的 Linux/macOS/Windows 平台块激活
2. **定义裸机平台宏** — `USE_MALLOC`、`GETPAGESIZE()`、`IEEE_DOUBLE` 等

```c
// baremetal_pre.h（在 version.h 之前执行）
#undef __linux__           // 阻止 Linux 平台块
#define __BAREMETAL_RV64__
#define USE_MALLOC         // 用 malloc 替代 mmap
#define GETPAGESIZE() 4096
#define IEEE_DOUBLE
// ... 其他平台定义
```

由于 `-include` 在所有 `#include` 之前生效，当 `c/version.h` 被加载时，`__linux__` 已经被 undef，不会进入 Linux 块。而我们的平台定义已经就位，`version.h` 底部的 "Defaults and derived" 段正常处理。

#### `chez/stats_wrapper.c`（包装 stats.c）

原始 `c/version.h` 无条件定义 `USE_DEV_URANDOM_UUID`（在所有平台块之后），这会让 `stats.c` 尝试打开 `/dev/urandom`（裸机不可用）。解决方案：不直接编译 `c/stats.c`，而是通过包装文件先 `#undef USE_DEV_URANDOM_UUID`，再 `#include "stats.c"`，让它走 `uuid_generate()` 路径（由我们的 `uuid/uuid.h` 桩提供）。

**为什么用 `USE_MALLOC`**：Chez Scheme 支持三种内存分配策略：`USE_MMAP`（Linux 默认）、`USE_VIRTUAL_ALLOC`（Windows）、`USE_MALLOC`（Emscripten）。裸机没有 mmap，所以使用 malloc 策略，由我们的 `libc/memory.c` 提供实现。

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
| 左/右箭头 | 移动光标（UTF-8 字符为单位） |
| 上/下箭头 | 浏览历史命令（最多 64 条） |
| Backspace | 删除光标前的字符（UTF-8 感知） |
| Delete | 删除光标后的字符（UTF-8 感知） |
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
  ├── boot.o, uart.o, trap.o, kernel_main.o      (裸机骨架)
  ├── libgcc_override.o                            (libgcc 函数覆盖)
  ├── libc/*.o                                     (19 个 freestanding libc)
  ├── chez_objs/{scheme,alloc,segment,...}.o       (28 个 Chez C 运行时)
  ├── chez_objs/{expeditor_stub,stats_wrapper}.o   (平台适配桩)
  ├── chez_objs/zlib_*.o                           (15 个 zlib 压缩库)
  ├── chez_objs/lz4_*.o                            (4 个 lz4 压缩库)
  ├── chez_objs/{petite_boot,scheme_boot}.o        (嵌入的 boot 文件)
  └── init_ss.o                                     (嵌入的 Scheme 初始化脚本)
```

**两组不同的编译标志**：
- `CFLAGS`：用于裸机代码（boot/kernel/libc），使用 `-isystem libc/include`
- `CHEZ_CFLAGS`：用于 Chez C 运行时，额外使用 `-include chez/baremetal_pre.h` 注入平台定义

**链接**：使用自定义链接脚本 `linker.ld`，链接所有目标文件和 `libgcc.a`（提供软件除法等）。

**纯 RV64G 编译**：所有代码使用 `-march=rv64g`（不含 C 压缩扩展）编译，确保只生成 32 位指令。QEMU 通过 `-cpu rv64,c=false` 禁用 C 扩展。

### 9. Boot 文件交叉编译

`os/boot/` 下的 rv64le boot 文件通过 Chez Scheme 的构建系统从 pb（portable bytecode）交叉编译而来：

```bash
# 在 ChezScheme 根目录执行：
./configure -m=rv64le --toolprefix=riscv64-linux-gnu- \
  CC_FOR_BUILD=gcc --as-is --disable-curses --disable-x11 --disable-iconv
make rv64le.bootquick
# 将输出复制到 os/boot/
cp boot/rv64le/{petite.boot,scheme.boot,scheme.h,equates.h,gc-*.inc,heapcheck.inc} os/boot/
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

### 11. 自动化测试 (`tests/run_tests.sh`)

30 个测试用例，分为 8 组：

| 分组 | 测试内容 |
|------|---------|
| Boot/REPL | 版本显示、提示符、算术、大数、无崩溃 |
| Display | 字符串输出、多行、换行 |
| UTF-8 | 中文显示、字符串长度、字符提取 |
| Error | 除零错误后 REPL 恢复、无崩溃 |
| GC | 分配 + 回收压力测试 |
| System | machine-type、scheme-version |
| Echo | 输入回显、结果正确 |
| Timer | 定时器创建、触发、取消、列表查询 |

每组启动一个 QEMU 实例，含耗时统计。

## 关键设计决策

| 决策 | 原因 |
|------|------|
| 所有改动在 `os/` 下 | 不修改原始 Chez Scheme 源码，便于跟踪上游更新 |
| `-include baremetal_pre.h` | 通过编译器参数注入平台定义，无需修改 `c/version.h` |
| `stats_wrapper.c` 包装 | 绕过 `USE_DEV_URANDOM_UUID` 而不修改 `c/stats.c` |
| 使用 `USE_MALLOC` 而非 `USE_MMAP` | 裸机没有 mmap 系统调用，malloc 是最简单的策略 |
| 非线程构建 (`rv64le` 而非 `trv64le`) | 消除 pthread 依赖，裸机不需要多线程 |
| 嵌入 boot 文件到 ELF | 避免需要文件系统来加载 boot 文件 |
| 行缓冲 stdin | 确保按回车后才求值，而非括号匹配后立刻求值 |
| `ee_init_term` 返回 0 | 告诉 Chez Scheme 不使用 expeditor，回退到基本 I/O |
| FPU 初始化在 boot.S | RV64G 的 FPU 默认关闭，不初始化会触发非法指令异常 |
| 纯 RV64G（无 C 扩展） | 确保所有指令为 32 位，简化硬件实现和调试 |
| libgcc 函数覆盖 | 系统 libgcc 含压缩指令，在纯 RV64G 上会触发非法指令 |
| UTF-8 感知行编辑 | 支持中文等多字节字符的输入、删除和光标移动 |
| Scheme优先 | 业务逻辑尽量用 Scheme 实现，C 层只提供原语 |
| init.ss独立文件 | Scheme 初始化脚本独立维护，不硬编码在 C 字符串中 |
| GC安全的文件加载 | 使用 Slock_object/Sunlock_object 保护加载期间的字符串对象 |

## C 层原语 API 参考

以下是 C 层通过 `Sforeign_symbol` 注册的所有外部接口，可在 Scheme 中通过 `foreign-procedure` 调用。**新功能应只使用这些原语，不需要修改 C 代码。**

### 系统信息 (`sysinfo.c`)

| C 符号 | Scheme 声明 | 说明 |
|--------|------------|------|
| `c_sysinfo_data` | `(foreign-procedure "c_sysinfo_data" () ptr)` | 返回 13 元素 vector |

返回的 vector 字段：

| 索引 | 含义 | 数据来源 |
|------|------|----------|
| 0 | misa | CSR `misa`（ISA 扩展位图） |
| 1 | mvendorid | CSR `mvendorid` |
| 2 | marchid | CSR `marchid` |
| 3 | mimpid | CSR `mimpid` |
| 4 | mhartid | CSR `mhartid`（硬件线程 ID） |
| 5 | mtime | `rdtime` 计时器（10 MHz） |
| 6 | heap-start | 链接脚本 `_heap_start` 地址 |
| 7 | heap-end | 链接脚本 `_heap_end` 地址 |
| 8 | stack-size | `_stack_top - _stack_bottom`（字节） |
| 9 | bss-size | `_bss_end - _bss_start`（字节） |
| 10 | code-size | `_bss_start - 0x80000000`（字节） |
| 11 | petite-size | petite.boot 大小（字节） |
| 12 | scheme-size | scheme.boot 大小（字节） |

```scheme
;; 使用示例
(define c-sysinfo-data (foreign-procedure "c_sysinfo_data" () ptr))
(let ((d (c-sysinfo-data)))
  (display (string-append "heap: "
    (number->string (vector-ref d 6) 16) " - "
    (number->string (vector-ref d 7) 16) "\n")))
```

### 定时器 (`timer.c`)

| C 符号 | Scheme 声明 | 说明 |
|--------|------------|------|
| `scheme_set_timer` | `(foreign-procedure "scheme_set_timer" (ptr ptr ptr) ptr)` | 创建定时器。参数：秒数、回调、是否重复。返回 timer-id 或 `#f` |
| `scheme_cancel_timer` | `(foreign-procedure "scheme_cancel_timer" (ptr) ptr)` | 取消定时器。参数：timer-id |
| `scheme_timer_list` | `(foreign-procedure "scheme_timer_list" () ptr)` | 返回活跃 timer 列表（见下） |
| `scheme_timer_max_slots` | `(foreign-procedure "scheme_timer_max_slots" () ptr)` | 返回最大 timer 槽位数（fixnum 16） |

`scheme_timer_list` 返回一个 Scheme list，每个元素为 5 元素 vector：

| 索引 | 含义 | 类型 |
|------|------|------|
| 0 | timer-id | fixnum |
| 1 | repl-id | fixnum |
| 2 | interval（秒） | integer（0 = 一次性） |
| 3 | remaining（秒） | integer |
| 4 | repeat? | boolean |

```scheme
;; 使用示例
(define c-timer-list (foreign-procedure "scheme_timer_list" () ptr))
(for-each
  (lambda (t)
    (display (string-append "#" (number->string (vector-ref t 0))
              " remaining: " (number->string (vector-ref t 3)) "s\n")))
  (c-timer-list))
```

### REPL 支持 (`stdio.c` / `timer.c`)

| C 符号 | Scheme 声明 | 说明 |
|--------|------------|------|
| `stdio_set_prompt` | `(foreign-procedure "stdio_set_prompt" (string) void)` | 设置行编辑器提示符 |
| `timer_set_current_repl` | `(foreign-procedure "timer_set_current_repl" (int) void)` | 设置当前 REPL ID（用于 timer 输出缓冲） |
| `timer_flush_repl_buffer` | `(foreign-procedure "timer_flush_repl_buffer" () void)` | 刷出当前 REPL 的缓冲输出 |

```scheme
;; 使用示例：切换 REPL 时的标准流程
(define c-set-prompt (foreign-procedure "stdio_set_prompt" (string) void))
(define c-set-repl-id (foreign-procedure "timer_set_current_repl" (int) void))
(define c-flush-repl-buf (foreign-procedure "timer_flush_repl_buffer" () void))

(c-set-prompt "[1]> ")
(c-set-repl-id 1)
(c-flush-repl-buf)  ; 刷出 REPL 1 的缓冲 timer 输出
```

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
| 新增文件 | 90 个（全部在 `os/` 下） |
| 裸机核心代码 | 10 个文件 (boot.S, linker.ld, kernel_main.c, sysinfo.c, timer.c, uart.c, trap.c, uart.h, libgcc_override.c, scheme/init.ss) |
| Freestanding libc | 19 个源文件 + 44 个头文件 |
| Chez 平台适配 | 4 个文件 (baremetal_pre.h, config.h, expeditor_stub.c, stats_wrapper.c) |
| 引导文件 | 8 个文件 (petite.boot, scheme.boot, scheme.h, equates.h, gc-*.inc) |
| 测试 | 30 个自动化用例 |
| Chez C 运行时编译 | 28 个文件 |
| 压缩库编译 | zlib 15 个 + lz4 4 个 |
| 指令集 | 纯 RV64G（无 C 压缩扩展），全部 32 位指令 |
| 内核大小 | ~5.3MB (含 3.4MB 嵌入 boot 文件) |
| QEMU 内存 | 256MB |
| 原始 Chez Scheme 修改 | **零** |
