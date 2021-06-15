section .kmain
global kmain
kmain:
  extern main
  call main
  cli
end_loop:
  hlt
  jmp end_loop

