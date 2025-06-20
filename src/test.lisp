(defun test (num)
  (when (gte num 0)
    (print num " ")
    (test (sub num 1))
    )
  )


(test 1000)
