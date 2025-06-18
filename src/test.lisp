(defun sayhello (name times)
  (when (gt times 0)
    (print "Hello, " name "\n")
    (sayhello (sub times 1) (sub times 1))    
    (print "Goodbye, " name "\n")
    )
  )

(sayhello "bob" 12)
