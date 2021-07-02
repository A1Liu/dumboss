.text
.code64

.global _x86_64_asm_set_cs
.p2align 4
_x86_64_asm_set_cs:
    pushq %rdi
    leaq  1f(%rip), %rax
    pushq %rax
    lretq
1:
    retq
