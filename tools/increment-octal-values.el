(defun increment-octal-values ()
  "Increment octal values in defOp lines sequentially."
  (interactive)
  (save-excursion
    (goto-char (point-min))
    (let ((value #o500)) ;; Starting value in octal
      (while (re-search-forward "defOp(\\(0[0-7]+\\)," nil t)
        (replace-match (format "defOp(%04o," value))
        (setq value (1+ value)))))) ;; Increment octal value
