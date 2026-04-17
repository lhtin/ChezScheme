# ChezSchemeOS 开发规范

## 项目概述

ChezSchemeOS 是运行在 RV64G 裸机上的 Chez Scheme 操作系统。所有代码在 `os/` 目录下，不修改原始 Chez Scheme 源码。

## 开发流程

### 每次修改必须遵循

1. **编译验证**：`cd os && make clean && make` 确保零 error
2. **运行测试**：`cd os && make test` 确保 23/23 全部通过
3. **功能验证**：`cd os && ./run.sh` 手动验证交互行为（如涉及 REPL/输入）
4. **更新文档**：如果改动涉及新文件、新功能、架构变化，更新 `os/ARCHITECTURE.md`
5. **单独提交**：每个 feature/bugfix 单独一个 commit，不混合多个不相关改动

### Commit 规范

```
<type>: <简短描述>

<详细说明（如有必要）>
```

类型：
- `feat`: 新功能
- `fix`: Bug 修复
- `refactor`: 重构（不改变行为）
- `docs`: 文档更新
- `test`: 测试相关
- `chore`: 构建/工具变更

### 新增测试

如果新功能影响了 REPL 行为、I/O、UTF-8 处理、启动流程等，需要在 `os/tests/run_tests.sh` 中添加对应测试用例。

## 目录结构约定

```
os/
├── boot.S, linker.ld, kernel_main.c   核心启动代码
├── uart.c, trap.c                      硬件驱动
├── libgcc_override.c                   libgcc 替换函数
├── boot/                               rv64le 引导文件（交叉编译产物）
├── chez/                               Chez Scheme 平台适配层
│   ├── baremetal_pre.h                 通过 -include 注入的平台定义
│   ├── config.h                        裸机配置
│   ├── expeditor_stub.c               编辑器桩函数
│   └── stats_wrapper.c                stats.c 包装
├── libc/                               Freestanding C 库
│   ├── *.c                            实现文件
│   ├── setjmp.S                       汇编实现
│   └── include/                       头文件
├── tests/
│   └── run_tests.sh                   自动化测试
├── Makefile                           构建系统
├── run.sh                             QEMU 启动脚本
└── ARCHITECTURE.md                    技术文档
```

## 关键约束

### 尽可能在 Scheme 层面实现功能

新增功能应优先用 Scheme 实现，C 层只提供必要的硬件访问原语（UART、CLINT、CSR 操作等）。如果一个功能可以用 Scheme 完成，就不要用 C 来写。这样做的好处：
- 利用 Scheme 的高级抽象能力，代码更简洁
- 减少 C 层的复杂度和潜在内存安全问题
- 便于在 REPL 中动态修改和调试

### 不修改原始 Chez Scheme 源码

所有适配通过以下机制实现，**禁止修改 `os/` 外的任何文件**：
- `-include chez/baremetal_pre.h`：编译器参数注入平台定义
- `chez/stats_wrapper.c`：包装文件绕过 urandom 依赖
- `chez/expeditor_stub.c`：桩函数替代 curses 依赖
- `libc/include/`：freestanding 头文件替代系统头文件

### 纯 RV64G

- 编译使用 `-march=rv64g`（无 C 压缩扩展）
- QEMU 使用 `-cpu rv64,c=false`
- 如果引入新的 libgcc 依赖，检查是否含压缩指令，必要时在 `libgcc_override.c` 中提供替代实现

### 编译标志

- `CFLAGS`：用于 `os/` 下的裸机代码
- `CHEZ_CFLAGS`：用于编译 Chez Scheme 的 `c/*.c` 文件，包含 `-include chez/baremetal_pre.h`

## 常见开发场景

### 添加新的 libc 函数

1. 在 `libc/*.c` 中实现（选择语义最匹配的文件）
2. 在 `libc/include/*.h` 中声明
3. Makefile 的 `wildcard libc/*.c` 会自动发现新 .c 文件
4. `make clean && make && make test`

### 添加新的 Chez Scheme 桩函数

1. 如果是 foreign symbol：在 `chez/expeditor_stub.c` 中注册
2. 如果是 C 函数：在 `libc/` 对应文件中实现
3. 如果需要包装某个 Chez 源文件：参考 `chez/stats_wrapper.c` 模式

### 修改行编辑器

1. 编辑 `libc/stdio.c` 中的 `collect_line()` 函数
2. UTF-8 相关函数在同文件顶部（`utf8_char_len`、`utf8_display_width` 等）
3. 必须测试：ASCII 输入、中文输入、退格、方向键、历史

### 增加内存或修改内存布局

1. 修改 `linker.ld` 中的 `_heap_end` 值
2. 同步修改 `run.sh` 中的 `-m` 参数
3. 更新 `ARCHITECTURE.md` 中的内存布局图

## 构建依赖

- `riscv64-linux-gnu-gcc`：交叉编译器
- `qemu-system-riscv64`：模拟器
- Chez Scheme 源码树：`os/` 的父目录（`../c/`、`../zlib/`、`../lz4/`）
