;;; init.ss -- ChezSchemeOS initialization
;;;
;;; Loaded at startup by kernel_main.c after Chez Scheme heap is built
;;; and all foreign symbols are registered.

;;; ============================================================
;;; Utilities
;;; ============================================================

(define (clear)
  (display "\x1b;[2J\x1b;[H")
  (void))

(define sysinfo
  (let ((f (foreign-procedure "sysinfo_print" () void)))
    (lambda () (f) (void))))

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
;;; Timer Scheme bindings
;;; ============================================================

(define set-timer
  (let ((f (foreign-procedure "scheme_set_timer" (ptr ptr ptr) ptr)))
    (case-lambda
      ((s cb) (f s cb #f))
      ((s cb repeat) (f s cb repeat)))))

(define cancel-timer
  (let ((f (foreign-procedure "scheme_cancel_timer" (ptr) ptr)))
    (lambda (id) (f id))))

(define timer-info
  (let ((f (foreign-procedure "scheme_timer_info" () void)))
    (lambda () (f) (void))))

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
