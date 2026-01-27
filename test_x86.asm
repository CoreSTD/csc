; nasm -f elf32 test_x86.asm -o t.o
; ld -m elf_i386 -o t t.o
section .bss
    noob resd 100

section .text
    global _start

_start:
    xor eax, eax
    mov eax, 3
    mov ebx, 0
    lea ecx, [noob]
    mov edx, 100
    int 0x80

    mov eax, 4
    mov ebx, 1
    lea ecx, [noob]
    mov edx, 100
    int 0x80

    mov eax, 1
    mov ebx, 0
    int 0x80