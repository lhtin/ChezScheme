#!/bin/bash
# run_tests.sh -- Automated test suite for ChezSchemeOS
#
# Tests focus on: boot, REPL behavior, I/O, UTF-8 handling
# Starts QEMU once, feeds all expressions, checks full output.
#
# Usage: ./tests/run_tests.sh [kernel.elf]
# Exit code: 0 = all pass, 1 = some failed

set -u

KERNEL="${1:-kernel.elf}"
QEMU="qemu-system-riscv64"
QEMU_ARGS="-machine virt -cpu rv64,c=false -bios none -kernel $KERNEL -nographic -smp 1 -m 256M"
BOOT_WAIT=8

PASS=0
FAIL=0
ERRORS=""
START_TIME=$(date +%s)

echo "========================================"
echo "  ChezSchemeOS Test Suite"
echo "========================================"
echo ""

if [ ! -f "$KERNEL" ]; then
    echo "ERROR: $KERNEL not found. Run 'make' first."
    exit 1
fi

# --- Helper: run QEMU with given input, return output ---
run_qemu() {
    local input_cmds="$1"
    local total_wait="$2"
    eval "$input_cmds" | timeout "$total_wait" $QEMU $QEMU_ARGS 2>/dev/null
}

# --- Helper: check output contains expected string ---
check() {
    local desc="$1"
    local output="$2"
    local expect="$3"

    if echo "$output" | grep -qF "$expect"; then
        printf "  PASS  %s\n" "$desc"
        PASS=$((PASS + 1))
    else
        printf "  FAIL  %s\n" "$desc"
        printf "        expect: %s\n" "$expect"
        FAIL=$((FAIL + 1))
        ERRORS="${ERRORS}\n  - ${desc}"
    fi
}

check_not() {
    local desc="$1"
    local output="$2"
    local reject="$3"

    if echo "$output" | grep -qF "$reject"; then
        printf "  FAIL  %s\n" "$desc"
        printf "        should NOT contain: %s\n" "$reject"
        FAIL=$((FAIL + 1))
        ERRORS="${ERRORS}\n  - ${desc}"
    else
        printf "  PASS  %s\n" "$desc"
        PASS=$((PASS + 1))
    fi
}

# ============================================================
# TEST 1: Boot and REPL
# ============================================================
echo "[1] Boot and REPL basics"

T1_START=$(date +%s)
OUTPUT1=$(run_qemu '{ sleep '"$BOOT_WAIT"'; printf "(+ 1 2)\r"; sleep 2; printf "(* 6 7)\r"; sleep 2; printf "(expt 2 64)\r"; sleep 2; }' 20)
T1_END=$(date +%s)
echo "    ($(( T1_END - T1_START ))s)"

check "Boot: shows version banner" "$OUTPUT1" "Chez Scheme Version"
check "Boot: shows help hint" "$OUTPUT1" "Type (help)"
check "Boot: shows prompt" "$OUTPUT1" "> "
check "REPL: (+ 1 2) => 3" "$OUTPUT1" "3"
check "REPL: (* 6 7) => 42" "$OUTPUT1" "42"
check "REPL: bignum (expt 2 64)" "$OUTPUT1" "18446744073709551616"
check_not "Boot: no crash/trap" "$OUTPUT1" "*** TRAP ***"
check_not "Boot: no abnormal exit" "$OUTPUT1" "abnormal exit"

# ============================================================
# TEST 2: Display and output
# ============================================================
echo ""
echo "[2] Display and output"

T2_START=$(date +%s)
OUTPUT2=$(run_qemu '{ sleep '"$BOOT_WAIT"'; printf "(display \"hello world\")\r"; sleep 2; printf "(begin (display \"line1\") (newline) (display \"line2\") (newline))\r"; sleep 2; }' 18)
T2_END=$(date +%s)
echo "    ($(( T2_END - T2_START ))s)"

check "Display: hello world" "$OUTPUT2" "hello world"
check "Display: multi-line output" "$OUTPUT2" "line1"
check "Display: newline works" "$OUTPUT2" "line2"

# ============================================================
# TEST 3: UTF-8 handling
# ============================================================
echo ""
echo "[3] UTF-8 handling"

# \xe4\xbd\xa0\xe5\xa5\xbd = 你好
# \xe4\xb8\x96\xe7\x95\x8c = 世界
T3_START=$(date +%s)
OUTPUT3=$(run_qemu '{ sleep '"$BOOT_WAIT"'; printf "(display \"\xe4\xbd\xa0\xe5\xa5\xbd\")\r"; sleep 2; printf "(string-length \"\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c\")\r"; sleep 2; printf "(string-ref \"\xe4\xbd\xa0\xe5\xa5\xbd\" 0)\r"; sleep 2; }' 20)
T3_END=$(date +%s)
echo "    ($(( T3_END - T3_START ))s)"

check "UTF-8: display 你好" "$OUTPUT3" "你好"
check "UTF-8: string-length of 4 Chinese chars => 4" "$OUTPUT3" "4"
check "UTF-8: string-ref gets first char" "$OUTPUT3" "#\\你"

# ============================================================
# TEST 4: Error handling (REPL should recover, not crash)
# ============================================================
echo ""
echo "[4] Error recovery"

T4_START=$(date +%s)
OUTPUT4=$(run_qemu '{ sleep '"$BOOT_WAIT"'; printf "(/ 1 0)\r"; sleep 2; printf "(+ 1 2)\r"; sleep 2; }' 18)
T4_END=$(date +%s)
echo "    ($(( T4_END - T4_START ))s)"

check "Error: division by zero shows error" "$OUTPUT4" "Exception"
check "Error: REPL recovers after error" "$OUTPUT4" "3"
check_not "Error: no crash" "$OUTPUT4" "*** TRAP ***"

# ============================================================
# TEST 5: GC stress
# ============================================================
echo ""
echo "[5] Garbage collection"

T5_START=$(date +%s)
OUTPUT5=$(run_qemu '{ sleep '"$BOOT_WAIT"'; printf "(begin (collect) (let loop ((i 0)) (if (< i 100) (begin (make-vector 1000 0) (loop (+ i 1))))) (collect) (display \"gc-ok\") (newline))\r"; sleep 5; }' 20)
T5_END=$(date +%s)
echo "    ($(( T5_END - T5_START ))s)"

check "GC: survives alloc+collect cycle" "$OUTPUT5" "gc-ok"
check_not "GC: no crash" "$OUTPUT5" "*** TRAP ***"

# ============================================================
# TEST 6: Machine type and system info
# ============================================================
echo ""
echo "[6] System info"

T6_START=$(date +%s)
OUTPUT6=$(run_qemu '{ sleep '"$BOOT_WAIT"'; printf "(machine-type)\r"; sleep 2; printf "(string? (scheme-version))\r"; sleep 2; printf "(sysinfo)\r"; sleep 3; }' 20)
T6_END=$(date +%s)
echo "    ($(( T6_END - T6_START ))s)"

check "System: machine-type => rv64le" "$OUTPUT6" "rv64le"
check "System: scheme-version is string" "$OUTPUT6" "#t"
check "System: sysinfo shows CPU" "$OUTPUT6" "Architecture:   RV64"
check "System: sysinfo shows memory" "$OUTPUT6" "Total RAM:"
check "System: sysinfo shows uptime" "$OUTPUT6" "Uptime:"

# ============================================================
# TEST 7: Input echo and line editing
# ============================================================
echo ""
echo "[7] Input echo"

T7_START=$(date +%s)
# Type characters and check they appear in output (echo)
OUTPUT7=$(run_qemu '{ sleep '"$BOOT_WAIT"'; printf "(+ 100 200)\r"; sleep 2; }' 16)
T7_END=$(date +%s)
echo "    ($(( T7_END - T7_START ))s)"

check "Echo: input visible in output" "$OUTPUT7" "(+ 100 200)"
check "Echo: result correct" "$OUTPUT7" "300"

# ============================================================
# TEST 8: Timer
# ============================================================
echo ""
echo "[8] Timer"

T8_START=$(date +%s)
OUTPUT8=$(run_qemu '{ sleep '"$BOOT_WAIT"'; printf "(set-timer 2 (lambda () (display \"TIMER-OK\\n\")))\r"; sleep 1; printf "(timer-info)\r"; sleep 4; }' 20)
T8_END=$(date +%s)
echo "    ($(( T8_END - T8_START ))s)"

check "Timer: set-timer returns id" "$OUTPUT8" "1"
check "Timer: timer-info shows active" "$OUTPUT8" "Active Timers"
check "Timer: callback fires" "$OUTPUT8" "TIMER-OK"
check_not "Timer: no crash" "$OUTPUT8" "*** TRAP ***"

# ============================================================
# Summary
# ============================================================
END_TIME=$(date +%s)
TOTAL_TIME=$((END_TIME - START_TIME))

echo ""
echo "========================================"
TOTAL=$((PASS + FAIL))
echo "  Results: $PASS/$TOTAL passed, ${TOTAL_TIME}s total"
if [ $FAIL -gt 0 ]; then
    echo ""
    echo "  Failed tests:"
    printf "$ERRORS\n"
    echo "========================================"
    exit 1
else
    echo "  All tests passed!"
    echo "========================================"
    exit 0
fi
