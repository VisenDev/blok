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
