; nasm -f elf64 test_x86_64.asm -o t.o
; ld -o t t.o
section .bss
    noob resd 100

section .text
    global _start

_start:
    xor rax, rax
    mov rax, 0
    mov rdi, 0
    lea rsi, [noob]
    mov rdx, 100
    syscall

    mov rax, 1
    mov rdi, 1
    lea rsi, [noob]
    mov rdx, 100
    syscall

    mov rax, 60
    mov rdi, 0
    syscall