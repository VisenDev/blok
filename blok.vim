" Vim syntax file
" Language: blok
" Maintainer: Robert Burnett

if exists("b:current_syntax")
  finish
endif

" Comments
syntax match customComment "//.*$"
highlight link customComment Comment

" Strings
syntax region customString start='"' end='"' skip='\\"'
highlight link customString String

" Integers
syntax match customInteger /\v[0-9]+/ containedin=ALLBUT,customComment,customString
highlight link customInteger Number

" match an s-expr head without including the '(' or spaces
syntax match customHeadSymbol '\((\s*\)\@<=\(\k\|[-+*/!?<>]\)\(\k\|[-+*/!?<>]\)*' containedin=ALLBUT,customComment,customString
highlight link customHeadSymbol Keyword

" highlight the first symbol after a semicolon (ignoring any spaces)
syntax match customAfterSemicolon /\%(;\s*\)\@<=\(\k\|[-+*/!?<>]\)\(\k\|[-+*/!?<>]\)*/ containedin=ALLBUT,customComment,customString
highlight link customAfterSemicolon Keyword

" Parentheses
syntax match customParen /[()]/
highlight link customParen Delimiter

" highlight standalone semicolons as delimiters
syntax match customDelimiter /;/ containedin=ALLBUT,customComment,customString
highlight link customDelimiter Delimiter

" Keywords (optional)
syntax keyword customKeyword define lambda if else let
highlight link customKeyword Keyword

let b:current_syntax = "sexpr_language"

" Indentation settings
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
