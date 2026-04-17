# ChezSchemeOS API 文档

ChezSchemeOS 在标准 Chez Scheme REPL 基础上提供了以下系统命令。

## 快速开始

```bash
cd os
make clean && make    # 编译
./run.sh              # 启动（QEMU RV64G M-mode）
make test             # 运行自动化测试
```

QEMU 内按 `Ctrl-A` 然后 `X` 退出。

---

## 系统命令

### `(help)`

显示帮助信息，列出所有可用命令。

```scheme
> (help)

========================================
  ChezSchemeOS - Chez Scheme on RV64G
========================================

This is a full Chez Scheme REPL running
bare-metal on RISC-V 64-bit (M-mode).
...
```

---

### `(sysinfo)`

显示系统状态信息，包括 CPU、内存、引导文件、存储、运行时间和平台信息。

```scheme
> (sysinfo)

================== System Info ==================

  CPU
    Architecture:   RV64 (ADFHIMSU)
    Vendor ID:      0 (not implemented)
    Architecture ID: 0x0000000000000000
    Implementation: 0x0000000000000000
    Hart ID:        0
    Privilege:      M-mode (Machine)

  Memory
    Total RAM:      256 MB
    Code + Data:    4 MB
    BSS:            1 MB
    Stack:          1 MB
    Heap:           249 MB (available for malloc)

  Boot Files
    petite.boot:    2 MB
    scheme.boot:    1 MB

  Storage
    Disk:           none (no filesystem)
    Boot method:    embedded in kernel ELF

  Runtime
    Uptime:         42s
    Timer freq:     10 MHz

  Platform
    Machine:        QEMU virt
    UART:           NS16550A @ 0x10000000
    OS:             ChezSchemeOS
    Scheme:         Chez Scheme 10.4.0
    Machine type:   rv64le

=================================================
```

**信息来源**：CPU 信息从 RISC-V `misa`/`mvendorid`/`marchid` 等 CSR 寄存器读取；内存信息从链接脚本符号计算；运行时间基于 `rdtime` 硬件计时器。

---

## 定时器

ChezSchemeOS 支持 M-mode 硬件定时器中断，最多可同时运行 16 个定时器。回调函数在主循环中安全执行（非中断上下文），可以调用任意 Scheme 代码。

### `(set-timer seconds callback)`

创建一个**一次性**定时器。到期后执行回调一次，然后自动移除。

- **seconds**：定时时间（秒，整数）
- **callback**：到期时调用的无参过程
- **返回值**：timer-id（整数），用于 `cancel-timer`

```scheme
> (set-timer 5 (lambda () (display "5 seconds passed!\n")))
1
;; 等待 5 秒后自动输出：
5 seconds passed!
```

### `(set-timer seconds callback #t)`

创建一个**重复**定时器。每隔指定秒数执行一次回调，直到被取消。

- **seconds**：间隔时间（秒，整数）
- **callback**：每次触发时调用的无参过程
- **#t**：表示重复模式
- **返回值**：timer-id

```scheme
> (set-timer 1 (lambda () (display "tick ")) #t)
2
;; 每秒输出：
tick tick tick tick ...
```

### `(cancel-timer id)`

取消指定 ID 的定时器。

- **id**：`set-timer` 返回的 timer-id
- **返回值**：void

```scheme
> (define t (set-timer 1 (lambda () (display ".")) #t))
> . . . . .
> (cancel-timer t)
```

### `(timer-info)`

显示所有活跃的定时器信息。

```scheme
> (set-timer 10 (lambda () (display "A")))
1
> (set-timer 2 (lambda () (display "B")) #t)
2
> (timer-info)

================ Active Timers ================

  #1  one-shot   remaining: 7s  callback: #<procedure>
  #2  repeating  interval: 2s   remaining: 1s  callback: #<procedure>

  Slots: 2/16 used

===============================================
```

**字段说明**：
- **#N**：timer-id
- **one-shot / repeating**：类型
- **remaining**：距离下次触发的时间
- **interval**：重复间隔（仅 repeating）
- **callback**：回调过程
- **Slots**：已用/总槽位

---

## Chez Scheme 内置命令

以下是 Chez Scheme 自带的常用命令（非 ChezSchemeOS 特有）：

### `(machine-type)`

返回当前机器类型。

```scheme
> (machine-type)
rv64le
```

### `(scheme-version)`

返回 Chez Scheme 版本字符串。

```scheme
> (scheme-version)
"10.4.0-pre-release.4"
```

### `(collect)`

手动触发垃圾回收。

```scheme
> (collect)
```

### `(exit)`

退出 Scheme（在裸机上会导致系统停机）。

```scheme
> (exit)
```

---

## 行编辑快捷键

REPL 提供类 readline 的行编辑功能：

| 按键 | 功能 |
|------|------|
| `←` / `→` | 移动光标（UTF-8 字符为单位） |
| `↑` / `↓` | 浏览历史命令（最多 64 条） |
| `Backspace` | 删除光标前的字符 |
| `Delete` | 删除光标处的字符 |
| `Home` / `End` | 跳到行首/行尾 |
| `Ctrl-A` | 跳到行首 |
| `Ctrl-E` | 跳到行尾 |
| `Ctrl-K` | 删除到行尾 |
| `Ctrl-U` | 清空整行 |
| `Ctrl-C` | 取消当前输入 |
| `Ctrl-D` | 空行时退出 |
| `Enter`（空行） | 重新显示 `>` 提示符 |

完整支持 UTF-8 多字节字符（中文、日文、韩文等）的输入、删除和光标移动。CJK 字符正确占 2 个显示宽度。

---

## 使用示例

### 基本运算

```scheme
> (+ 1 2)
3
> (* 6 7)
42
> (expt 2 64)
18446744073709551616
```

### 函数定义

```scheme
> (define (factorial n)
    (if (<= n 1) 1 (* n (factorial (- n 1)))))
> (factorial 20)
2432902008176640000
```

### 列表操作

```scheme
> (map (lambda (x) (* x x)) '(1 2 3 4 5))
(1 4 9 16 25)
> (filter odd? '(1 2 3 4 5 6))
(1 3 5)
```

### UTF-8 字符串

```scheme
> (display "你好世界\n")
你好世界
> (string-length "你好")
2
```

### 定时器应用：简易时钟

```scheme
> (set-timer 1
    (lambda ()
      (let ((t (current-time)))
        (display (date->string (current-date) "~H:~M:~S"))
        (display "\r")))
    #t)
```

### 定时器应用：延迟执行

```scheme
> (display "start\n")
start
> (set-timer 3 (lambda () (display "done!\n")))
1
;; 3 秒后输出 done!
done!
```
