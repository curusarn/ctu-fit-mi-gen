let SessionLoad = 1
if &cp | set nocp | endif
let s:so_save = &so | let s:siso_save = &siso | set so=0 siso=0
let v:this_session=expand("<sfile>:p")
silent only
cd ~/git/ctu-fit-mi-gen/src
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
set shortmess=aoO
badd +18 compiler.cpp
badd +291 compiler.h
badd +0 mila/ast.cpp
badd +0 mila/ast.h
argglobal
silent! argdel *
$argadd compiler.cpp
$argadd compiler.h
set stal=2
edit compiler.h
set splitbelow splitright
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd w
wincmd _ | wincmd |
split
1wincmd k
wincmd w
set nosplitbelow
set nosplitright
wincmd t
set winminheight=1 winheight=1 winminwidth=1 winwidth=1
exe 'vert 1resize ' . ((&columns * 119 + 119) / 239)
exe '2resize ' . ((&lines * 27 + 28) / 57)
exe 'vert 2resize ' . ((&columns * 119 + 119) / 239)
exe '3resize ' . ((&lines * 26 + 28) / 57)
exe 'vert 3resize ' . ((&columns * 119 + 119) / 239)
argglobal
if bufexists('compiler.h') | buffer compiler.h | else | edit compiler.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 352 - ((34 * winheight(0) + 27) / 54)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
352
normal! 09|
wincmd w
argglobal
if bufexists('compiler.cpp') | buffer compiler.cpp | else | edit compiler.cpp | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 18 - ((13 * winheight(0) + 13) / 27)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
18
normal! 0
wincmd w
argglobal
if bufexists('compiler.h') | buffer compiler.h | else | edit compiler.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 456 - ((12 * winheight(0) + 13) / 26)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
456
normal! 0
wincmd w
exe 'vert 1resize ' . ((&columns * 119 + 119) / 239)
exe '2resize ' . ((&lines * 27 + 28) / 57)
exe 'vert 2resize ' . ((&columns * 119 + 119) / 239)
exe '3resize ' . ((&lines * 26 + 28) / 57)
exe 'vert 3resize ' . ((&columns * 119 + 119) / 239)
tabedit mila/ast.h
set splitbelow splitright
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd w
set nosplitbelow
set nosplitright
wincmd t
set winminheight=1 winheight=1 winminwidth=1 winwidth=1
exe 'vert 1resize ' . ((&columns * 119 + 119) / 239)
exe 'vert 2resize ' . ((&columns * 119 + 119) / 239)
argglobal
2argu
if bufexists('mila/ast.h') | buffer mila/ast.h | else | edit mila/ast.h | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 215 - ((22 * winheight(0) + 27) / 54)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
215
normal! 09|
wincmd w
argglobal
2argu
if bufexists('mila/ast.cpp') | buffer mila/ast.cpp | else | edit mila/ast.cpp | endif
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let s:l = 146 - ((34 * winheight(0) + 27) / 54)
if s:l < 1 | let s:l = 1 | endif
exe s:l
normal! zt
146
normal! 0
wincmd w
exe 'vert 1resize ' . ((&columns * 119 + 119) / 239)
exe 'vert 2resize ' . ((&columns * 119 + 119) / 239)
tabnext 1
set stal=1
if exists('s:wipebuf')
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 shortmess=filnxtToO
set winminheight=1 winminwidth=1
let s:sx = expand("<sfile>:p:r")."x.vim"
if file_readable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &so = s:so_save | let &siso = s:siso_save
let g:this_session = v:this_session
let g:this_obsession = v:this_session
let g:this_obsession_status = 2
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
