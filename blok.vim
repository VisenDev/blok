" Vim syntax file
" Language: blok
" Maintainer: Robert Burnett

if exists("b:current_syntax")
  finish
endif

" Comments
syntax match blokComment "//.*$"
highlight link blokComment Comment

" Strings
syntax region blokString start='"' end='"' skip='\\"'
highlight link blokString String

" Integers
syntax match blokInteger /\v[0-9]+/ containedin=ALLBUT,blokComment,blokString
highlight link blokInteger Number


" Keyvalue delimiter
syntax match blokColon /:/ 
highlight link blokColon Keyword

" First element in sexpr
"syntax match blokSexprHeader '\((\s*\)\@<=\(\k\)\(\k\)*' 
"highlight link blokSexprHeader Keyword

" Parentheses
syntax match blokParen /[()]/ containedin=ALLBUT,blokSexprHeader
highlight link blokParen Delimiter 

" Types
syntax match blokType /\<[A-Z][A-Za-z0-9_]*\>[\[\]]*/
highlight link blokType Identifier




let b:current_syntax = "blok"




""""""" Indentation settings """""""
if exists("b:did_indent")
  finish
endif
let b:did_indent = 1

setlocal indentexpr=GetSexprIndent(v:lnum)
setlocal indentkeys+=0)

function! GetSexprIndent(lnum)
  let lnum = a:lnum
  let thisline = getline(lnum)

  if thisline =~ '^\s*\($\|//\)'
    return indent(lnum - 1)
  endif

  let plnum = lnum - 1
  while plnum > 0
    let pline = getline(plnum)
    if pline !~ '^\s*\($\|//\)'
      break
    endif
    let plnum -= 1
  endwhile

  if plnum <= 0
    return 0
  endif

  let line_to_check = getline(plnum)
  let open_parens = len(line_to_check) - len(substitute(line_to_check, '(', '', 'g'))
  let close_parens = len(line_to_check) - len(substitute(line_to_check, ')', '', 'g'))

  if open_parens > close_parens
    return indent(plnum) + 4
  elseif open_parens < close_parens
    return max([indent(plnum) - 4, 0])
  else
    return indent(plnum)
  endif
endfunction
