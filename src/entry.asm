section .kmain
global kmain
kmain:
  ; Input  calling convention is x86_64-unknown-windows
  ;        https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-160#parameter-passing
  ; Output calling convention is x86_64-unkown-elf
  ;        https://stackoverflow.com/questions/2535989/what-are-the-calling-conventions-for-unix-linux-system-calls-and-user-space-f
  mov rdi, rcx ; first parameter is in rcx in windows, and in rdi in elf
  extern main
  call main
  cli

end_loop:
  hlt
  jmp end_loop

