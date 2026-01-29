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
        syscall = 3,
        int_0x80 = 4
    } INSTRUCTIONS;

    #define SYSCALL     {0x0F, 0x05}
    #define INI_0x80    {0xCD, 0x80}
    #define RET         {0xC3}

    /* Current supported instructions */
    #define MAX_INSTRUCTIONS 4
    void INSTRUCTIONS[][2] = {
        {(void *)xor, "xor"},
        {(void *)mov, "mov"},
        {(void *)syscall, "syscall"},
        {(void *)int_0x80, "int 0x80"}
    };

    /* File Byte Indentications */
    const u8 NULL_TERMINATOR = '\0';

    /* The beginning of the string section */
    const u8 E_O_C = {0xFF, 0x00, 0xFF};

    /* 
        This goes after a string's length before the string in binary 
        example: 0xFF 0x00 0xFF 0xA2 0xFF --String--
    */
    const u8 BLACKSPACE = 0xFF;
    bool reached_end_of_code(u8 *binary)
    { return binary[3] ? mem_cpy(binary, E_O_C, 3) : false; }

#endif