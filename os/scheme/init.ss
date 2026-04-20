;;; init.ss -- ChezSchemeOS initialization
;;;
;;; Loaded at startup by kernel_main.c after Chez Scheme heap is built
;;; and all foreign symbols are registered.
;;;
;;; All OS-level features are implemented here in Scheme.
;;; The C layer only provides hardware access primitives.

;;; ============================================================
;;; Utilities
;;; ============================================================

(define (clear)
  (display "\x1b;[2J\x1b;[H")
  (void))

;;; ============================================================
;;; System information (sysinfo)
;;; ============================================================

;; C primitive returns #(misa mvendorid marchid mimpid mhartid mtime
;;   heap-start heap-end stack-size bss-size code-size petite-size scheme-size)
(define c-sysinfo-data (foreign-procedure "c_sysinfo_data" () ptr))

(define (format-hex n)
  (string-append "0x" (number->string n 16)))

(define (format-size bytes)
  (cond
    ((>= bytes (* 1024 1024 1024))
     (string-append (number->string (quotient bytes (* 1024 1024 1024))) " GB"))
    ((>= bytes (* 1024 1024))
     (string-append (number->string (quotient bytes (* 1024 1024))) " MB"))
    ((>= bytes 1024)
     (string-append (number->string (quotient bytes 1024)) " KB"))
    (else
     (string-append (number->string bytes) " B"))))

(define (misa-extensions misa)
  (let loop ((i 0) (acc '()))
    (if (= i 26)
      (list->string (reverse acc))
      (loop (+ i 1)
            (if (not (zero? (bitwise-and misa (bitwise-arithmetic-shift-left 1 i))))
              (cons (integer->char (+ (char->integer #\A) i)) acc)
              acc)))))

(define (sysinfo)
  (let* ((d (c-sysinfo-data))
         (misa (vector-ref d 0))
         (mvendorid (vector-ref d 1))
         (marchid (vector-ref d 2))
         (mimpid (vector-ref d 3))
         (mhartid (vector-ref d 4))
         (ticks (vector-ref d 5))
         (heap-start (vector-ref d 6))
         (heap-end (vector-ref d 7))
         (stack-sz (vector-ref d 8))
         (bss-sz (vector-ref d 9))
         (code-sz (vector-ref d 10))
         (petite-sz (vector-ref d 11))
         (scheme-sz (vector-ref d 12))
         (mxl (bitwise-arithmetic-shift-right misa 62))
         (arch-str (cond ((= mxl 1) "32") ((= mxl 2) "64") ((= mxl 3) "128") (else "??")))
         (total-ram (- heap-end #x80000000))
         (freq 10000000)
         (secs (quotient ticks freq))
         (mins (quotient secs 60))
         (hours (quotient mins 60)))

    (display "\n")
    (display "================== System Info ==================\n")
    (display "\n")

    ;; CPU
    (display "  CPU\n")
    (display (string-append "    Architecture:   RV" arch-str
              " (" (misa-extensions misa) ")\n"))
    (display (string-append "    Vendor ID:      "
              (if (zero? mvendorid) "0 (not implemented)" (format-hex mvendorid)) "\n"))
    (display (string-append "    Architecture ID: " (format-hex marchid) "\n"))
    (display (string-append "    Implementation: " (format-hex mimpid) "\n"))
    (display (string-append "    Hart ID:        " (number->string mhartid) "\n"))
    (display "    Privilege:      M-mode (Machine)\n")
    (display "\n")

    ;; Memory
    (display "  Memory\n")
    (display (string-append "    Total RAM:      " (format-size total-ram)
              " (0x80000000 - " (format-hex heap-end) ")\n"))
    (display (string-append "    Code + Data:    " (format-size code-sz) "\n"))
    (display (string-append "    BSS:            " (format-size bss-sz) "\n"))
    (display (string-append "    Stack:          " (format-size stack-sz) "\n"))
    (display (string-append "    Heap:           " (format-size (- heap-end heap-start))
              " (available for malloc)\n"))
    (display "\n")

    ;; Boot Files
    (display "  Boot Files\n")
    (display (string-append "    petite.boot:    " (format-size petite-sz) "\n"))
    (display (string-append "    scheme.boot:    " (format-size scheme-sz) "\n"))
    (display "\n")

    ;; Storage
    (display "  Storage\n")
    (display "    Disk:           none (no filesystem)\n")
    (display "    Boot method:    embedded in kernel ELF\n")
    (display "\n")

    ;; Runtime
    (display "  Runtime\n")
    (display (string-append "    Uptime:         "
              (if (> hours 0) (string-append (number->string hours) "h ") "")
              (if (> mins 0) (string-append (number->string (modulo mins 60)) "m ") "")
              (number->string (modulo secs 60)) "s\n"))
    (display "    Timer freq:     10 MHz\n")
    (display "\n")

    ;; Platform
    (display "  Platform\n")
    (display "    Machine:        QEMU virt\n")
    (display "    UART:           NS16550A @ 0x10000000\n")
    (display "    OS:             ChezSchemeOS\n")
    (display (string-append "    Scheme:         Chez Scheme " (scheme-version) "\n"))
    (display (string-append "    Machine type:   " (symbol->string (machine-type)) "\n"))
    (display "\n")
    (display "=================================================\n"))
  (void))

;;; ============================================================
;;; Multi-REPL system
;;; ============================================================

(define *repls* (list (interaction-environment)))
(define *repl-id* 0)

(define c-set-prompt (foreign-procedure "stdio_set_prompt" (string) void))
(define c-set-repl-id (foreign-procedure "timer_set_current_repl" (int) void))
(define c-flush-repl-buf (foreign-procedure "timer_flush_repl_buffer" () void))

(define (update-prompt)
  (let ((p (string-append "[" (number->string *repl-id*) "]> ")))
    (c-set-prompt p)
    (c-set-repl-id *repl-id*)
    (c-flush-repl-buf)
    (waiter-prompt-string p)))

(define (new-repl)
  (let ((env (copy-environment (interaction-environment) #t)))
    (set! *repls* (append *repls* (list env)))
    (set! *repl-id* (- (length *repls*) 1))
    (interaction-environment env)
    (update-prompt)
    (display (string-append "Created REPL #"
              (number->string *repl-id*)
              " (" (number->string (length *repls*)) " total)\n"))
    (void)))

(define (next-repl)
  (if (<= (length *repls*) 1) (void)
    (begin
      (set! *repl-id* (modulo (+ *repl-id* 1) (length *repls*)))
      (interaction-environment (list-ref *repls* *repl-id*))
      (update-prompt)
      (display (string-append "Switched to REPL #"
                (number->string *repl-id*)
                "/" (number->string (length *repls*)) "\n"))
      (void))))

(define (prev-repl)
  (if (<= (length *repls*) 1) (void)
    (begin
      (set! *repl-id* (modulo (- *repl-id* 1) (length *repls*)))
      (interaction-environment (list-ref *repls* *repl-id*))
      (update-prompt)
      (display (string-append "Switched to REPL #"
                (number->string *repl-id*)
                "/" (number->string (length *repls*)) "\n"))
      (void))))

(define (switch-repl n)
  (if (and (>= n 0) (< n (length *repls*)))
    (begin
      (set! *repl-id* n)
      (interaction-environment (list-ref *repls* *repl-id*))
      (update-prompt)
      (display (string-append "Switched to REPL #"
                (number->string *repl-id*) "\n"))
      (void))
    (display (string-append "Invalid: use 0-"
              (number->string (- (length *repls*) 1)) "\n"))))

(define (repl-list)
  (display "\nActive REPLs:\n")
  (let loop ((i 0) (rest *repls*))
    (when (pair? rest)
      (display (string-append
        (if (= i *repl-id*) "  * [" "    [")
        (number->string i) "]\n"))
      (loop (+ i 1) (cdr rest))))
  (display (string-append "  (" (number->string (length *repls*))
            " total, * = current)\n"))
  (void))

(define (close-repl)
  (if (<= (length *repls*) 1)
    (display "Cannot close the last REPL\n")
    (begin
      (set! *repls* (append
        (list-head *repls* *repl-id*)
        (list-tail *repls* (+ *repl-id* 1))))
      (set! *repl-id* (min *repl-id* (- (length *repls*) 1)))
      (interaction-environment (list-ref *repls* *repl-id*))
      (update-prompt)
      (display (string-append "Closed. Now on REPL #"
                (number->string *repl-id*) "/"
                (number->string (length *repls*)) "\n"))
      (void))))

;;; ============================================================
;;; Timer
;;; ============================================================

(define set-timer
  (let ((f (foreign-procedure "scheme_set_timer" (ptr ptr ptr) ptr)))
    (case-lambda
      ((s cb) (f s cb #f))
      ((s cb repeat) (f s cb repeat)))))

(define cancel-timer
  (let ((f (foreign-procedure "scheme_cancel_timer" (ptr) ptr)))
    (lambda (id) (f id))))

;; timer-info: C returns a Scheme list of #(id repl-id interval remaining repeat?)
(define c-timer-list (foreign-procedure "scheme_timer_list" () ptr))
(define c-timer-max-slots (foreign-procedure "scheme_timer_max_slots" () ptr))

(define (timer-info)
  (let ((timers (c-timer-list))
        (max-slots (c-timer-max-slots)))
    (display "\n")
    (display "================ Active Timers ================\n")
    (display "\n")
    (if (null? timers)
      (display "  (no active timers)\n")
      (for-each
        (lambda (t)
          (let ((id (vector-ref t 0))
                (repl-id (vector-ref t 1))
                (interval (vector-ref t 2))
                (remaining (vector-ref t 3))
                (repeat? (vector-ref t 4)))
            (display (string-append
              "  #" (number->string id)
              "  [repl " (number->string repl-id) "]"
              (if repeat?
                (string-append "  repeating  interval: " (number->string interval) "s")
                "  one-shot ")
              (if (> remaining 0)
                (string-append "  remaining: " (number->string remaining) "s")
                "  (pending)")
              "\n"))))
        timers))
    (display "\n")
    (display (string-append "  Slots: " (number->string (length timers))
              "/" (number->string max-slots) " used\n"))
    (display "\n")
    (display "===============================================\n"))
  (void))

;;; ============================================================
;;; Help
;;; ============================================================

(define (help)
  (display "\n")
  (display "========================================\n")
  (display "  ChezSchemeOS - Chez Scheme on RV64G\n")
  (display "========================================\n")
  (display "\n")
  (display "This is a full Chez Scheme REPL running\n")
  (display "bare-metal on RISC-V 64-bit (M-mode).\n")
  (display "\n")
  (display "Examples:\n")
  (display "  (+ 1 2)                  => 3\n")
  (display "  (* 6 7)                  => 42\n")
  (display "  (expt 2 64)              => bignum\n")
  (display "  (map car '((a 1) (b 2))) => (a b)\n")
  (display "  (let ((x 10)) (* x x))   => 100\n")
  (display "\n")
  (display "Commands:\n")
  (display "  (help)               show this help\n")
  (display "  (sysinfo)            system information\n")
  (display "  (clear)              clear screen\n")
  (display "  (set-timer s fn)     one-shot timer (seconds)\n")
  (display "  (set-timer s fn #t)  repeating timer\n")
  (display "  (cancel-timer id)    cancel a timer\n")
  (display "  (timer-info)         show active timers\n")
  (display "  (new-repl)           create new REPL\n")
  (display "  (repl-list)          list all REPLs\n")
  (display "  (switch-repl n)      switch to REPL #n\n")
  (display "  (close-repl)         close current REPL\n")
  (display "  (machine-type)       machine type\n")
  (display "  (scheme-version)     version string\n")
  (display "  (collect)            run GC\n")
  (display "\n")
  (display "Shortcuts: Ctrl-N next REPL, Ctrl-P prev REPL\n")
  (display "Exit QEMU: Ctrl-A then X\n")
  (display "\n")
  (void))

;;; ============================================================
;;; Initial prompt
;;; ============================================================

(waiter-prompt-string "[0]> ")
