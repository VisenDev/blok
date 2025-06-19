
(defun test (arg)
  (if (equal arg 1) 
    (print arg " is one\n")
    (print arg " is not one\n")
    )
  )

(defun fac (num)
  (add num num)
  )

(test 1)
(test 2)
(print (fac 10))
