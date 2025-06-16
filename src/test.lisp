(defun mytests ()
  (print "hello world from print fn" "\n")
  (print "\n")
  (print "hello again" "hi")
  (progn 
    (print "\t" "hi")
    (print 1 "\n")
    (print 2)
    )
  )

(defun twice (arg)
  (list arg arg)
  )

(defun sayhello (name)
  (print "hello, " name "\n")
  )

(print (twice 3) "\n")
(sayhello "john")
(sayhello "bob")

(defun equal_to_5 (arg)
  (print "Testing if " arg " is equal to 5: ")
  (when (equal 5 arg)
    (print "It is!")
    )
  (when (not (equal 5 arg))
    (print "It isn't :(")
    )
  (print "\n")
  )

(equal_to_5 4)
(equal_to_5 5)

(defun fib (num)
  (if (lte num 1)
    1
    (mul (fib (sub 1 num)) num)
    )
  )

(print (fib 5))

(print "hello hello\n")
