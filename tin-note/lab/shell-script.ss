#! scheme --script
;; 通过 ./shell-script.ss 执行
(for-each
  (lambda (x) (display x) (newline))
  (cdr (command-line)))