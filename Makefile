.PHONY: all

all: compile

compile:
	nasm -f elf32 test_x86.asm -o x86.o
	nasm -f elf64 test_x86_64.asm -o x86_64.o
	ld -m elf_i386 -o t x86.o
	ld -o t x86_64.o
	lbg t.c -o assembler