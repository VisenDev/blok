defun isspace [ch] [
  return [or [== ' ' ch] [== '\n' ch] ]
]


defun main [] [
  printf "Hello world\n"
  printf "%d" [isspace ' ']
  return 0
]
