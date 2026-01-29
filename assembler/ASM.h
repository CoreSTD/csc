#pragma once

#include <libbase.h>

#ifndef _LB_ASM_
    #define _LB_ASM_

    typedef enum
    {
        null_arch   = 0,
        x86         = 32,
        x86_64      = 64
    } arch_t;
    
    typedef enum {
        eax = 0xB8,
        ecx = 0xB9,
        edx = 0xBA,
        ebx = 0xBB,
        esp = 0xBC,
        ebp = 0xBD,
        esi = 0xBE,
        edi = 0xBF
    } regs;

    
    typedef struct {
        u8 opcode;
        const string x86;
        const string x64;
    } reg_info;

    #define REG_COUNT 8
    reg_info REGISTERS[] = {
        { eax, "eax", "rax" },
        { ebx, "ebx", "rbx" },
        { ecx, "ecx", "rcx" },
        { edx, "edx", "rdx" },
        { ebp, "ebp", "rbp" },
        { esp, "esp", "rsp" },
        { edi, "edi", "rdi" },
        { esi, "esi", "rsi" }
    };

    typedef enum
    {
        _no = 0,
        xor = 1,
        mov = 2,
        syscall = 1,
        int_0x80 = 2
    } INSTRUCTIONS;

    #define SYSCALL {0x0F, 0x05}
    #define INI_0x80 {0xCD, 0x80}
    #define RET {0xC3}

    #define MAX_INSTRUCTIONS 4
    void INSTRUCTIONS[][2] = {
        {(void *)xor, "xor"},
        {(void *)mov, "mov"},
        {(void *)syscall, "syscall"},
        {(void *)int_0x80, "int 0x80"}
    };

    const u8 NULL_TERMINATOR = '\0';
    const u8 NULL_END = 0x00;
    const u8 BLACKSPACE = 0xFF;

#endif