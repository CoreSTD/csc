#include <libbase.h>

const u8 NULL_TERMINATOR = '\0';
const u8 NULL_END = 0x00;
const u8 BLACKSPACE = 0xFF;

typedef enum
{
    null_arch   = 0,
    x86         = 1,
    x86_64      = 2
} arch_t;

arch_t ARCH_MODE;

// x86
typedef enum {
    eax = 0xB8,
    ecx = 0xB9,
    edx = 0xBA,
    ebx = 0xBB,
    esp = 0xBC,
    ebp = 0xBD,
    esi = 0xBE,
    edi = 0xBF
} mov;

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

u8 *zero_reg_32(mov reg) {
    u8 modrm = 0xC0 | ((reg - 0xB8) << 3) | (reg - 0xB8);
    return to_heap((u8 []){ 0x31, modrm }, 2);
}

u8 *zero_reg_64(mov reg) {
    u8 modrm = 0xC0 | ((reg - 0xB8) << 3) | (reg - 0xB8);
    return to_heap((u8 []){ 0x48, 0x31, modrm }, 3);
}

u8 *mov_imm32_reg(mov reg, u64 n) {
    return to_heap((u8 []){ reg , n & 0xFF, (n >> 8) & 0xFF, (n >> 16) & 0xFF, (n >> 24) & 0xFF}, sizeof(u8) * 5);
}

u8 *mov_imm64_reg(mov reg, u64 n) {
    return to_heap(
        (u8 []){ 
            0x48,
            reg, 
            n & 0xFF, 
            (n >> 8) & 0xFF, 
            (n >> 16) & 0xFF, 
            (n >> 24) & 0xFF, 
            (n >> 32) & 0xFF, 
            (n >> 40) & 0xFF, 
            (n >> 48) & 0xFF, 
            (n >> 56) & 0xFF 
        },
        sizeof(u8) * 10
    );
}

u8 *invoke_syscall() {
    return to_heap((u8 []){0x0F, 0x05}, sizeof(u8) * 2);
}

u8 *invoke_0x80() {
    return to_heap((u8 []){ 0xCD, 0x80 }, sizeof(u8) * 2);
}

// int hexstr_to_u8_strict(const char *s, u8 *out) {
//     if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
//         s += 2;

//     unsigned int value = 0;
//     int v;

//     while (*s) {
//         v = hexval(*s++);
//         if (v < 0)
//             return -1;
//         value = (value << 4) | v;
//         if (value > 0xFF)
//             return -1;
//     }

//     *out = (u8)value;
//     return 0;
// }

typedef struct 
{
    u8              *code;     // raw bytes
    int             needs_ptr; // on 'mov reg, raw_ptr'
    int             bytes;     // Bytes in 'code'
} opc;

opc OpCodes[1024] = {0};
int OpCodeCount = 0;

bool is_number(string q)
{
    for(int i = 0; q[i] != '\0'; i++)
    {
        if(q[i] < 0 && q[i] > 9)
            return false;
    }

    return true;
}

mov reg_to_type(string reg)
{
    for(int i = 0; i < REG_COUNT; i++)
    {
        if(str_cmp(REGISTERS[i].x86, reg) || str_cmp(REGISTERS[i].x64, reg))
            return REGISTERS[i].opcode;
    }

    return -1;
}

opc convert_asm(string q, ptr p)
{
    if(!q)
        return (opc){0};

    int argc = 0;
    sArr args = NULL;
    if(find_char(q, ' ') > -1)
        args = split_string(q, ' ', &argc);

    if(str_startswith(q, "xor"))
    {
        opc c = (opc){
            .code = ARCH_MODE == x86 ? zero_reg_32(reg_to_type(args[1])) : zero_reg_64(reg_to_type(args[1])),
            .bytes = ARCH_MODE == x86 ? 2 : 3,
            .needs_ptr = 0
        };
        OpCodes[OpCodeCount++] = c;
        return (opc){0};
    }
    // mov reg, 3
    // mov reg, 0x005320ad0402
    if(str_startswith(q, "mov"))
    {
        if(argc < 3)
            lb_panic("Invalid opcode...!");

        // Pointer Detected
        if(str_startswith(args[2], "0x"))
        {
            mov reg = reg_to_type(args[1]);
            string value = args[2]; // This needs to be checked for max number of i32
            OpCodes[OpCodeCount++] = (opc){
                .code = to_heap(
                    (u8 []){
                        0x48,
                        reg_to_type(args[1]),
                        0x69, 0x69, 0x69, 0x69,
                        0x00, 0x00, 0x00, 0x00
                    },
                    10
                ),
                // .code = c0de,
                .needs_ptr = 0,
                .bytes = 10
            };
            return (opc){0};
        }
        
        // Pushing a number to register (Must detect max number for imm32 or imm64)
        if(is_number(args[2]))
        {
            mov reg = reg_to_type(args[1]);
            char buff[100];
            byte_to_hex(reg, buff);
            _printf("Register: %s | ", buff);
            u64 value = str_to_int(args[2]); // This needs to be checked for max number of i32
            _printf("Num: %d \n", (void *)&value);
            // u8 *c0de = ARCH_MODE == x86 ? c0de = mov_imm32_reg(reg, value) : mov_imm64_reg(reg, value);
            
            u8 *c0de = mov_imm32_reg(reg, value);
            // if((i64)value >= -0x80000000LL && (i64)value <= 0x7FFFFFFFLL) 

            opc c = (opc){
                .code = c0de,
                .needs_ptr = 0,
                .bytes = 5
            };
            OpCodes[OpCodeCount++] = c;
        }
        return (opc){0};
    }

    // syscall
    if(str_startswith(q, "syscall"))
    {
        OpCodes[OpCodeCount++] = (opc){
            invoke_syscall(),
            0,
            2
        };
    }

    // int 0x80
    if(str_cmp(q, "int 0x80"))
    {
        OpCodes[OpCodeCount++] = (opc){
            invoke_0x80(),
            0,
            2
        };
    }

    return (opc){0};
}

// HEADERS:
//      file[0] == 'L' && file[1] == 'B'
//      u8 *file_type = file + 2; DYM || EXE
//      file + 5 == 0x32 || file + 5 == 0x64
// CODE SEGMENT:
//      ---------
// BUFFER:
//      SZ:CONST (in binary for load-up)
i8 entry(int argc, string argv[])
{
    if(argc > 1 && array_contains_str((array)argv, "--64"))
        ARCH_MODE = x86_64;
    else
        ARCH_MODE = x86;

    // Custom Indication
    OpCodes[OpCodeCount++] = (opc){
        .code = to_heap((u8 []){'L', 'B'}, sizeof(u8 *) * 2),
        .bytes = 2,
        .needs_ptr = 0
    };

    // Add File Type (DYM = Dynamic || EXE = Execute)
    OpCodes[OpCodeCount++] = (opc){
        .code = to_heap((u8 []){'E', 'X', 'E'}, sizeof(u8 *) * 3),
        .bytes = 3,
        .needs_ptr = 0
    };

    // Arch ( 0x32 Bit || 0x64 Bit)
    OpCodes[OpCodeCount++] = (opc){
        .code = to_heap((u8 []){0x32}, sizeof(u8 *) * 1),
        .bytes = 1,
        .needs_ptr = 0
    };

    convert_asm("xor rax, rax", 0);
    convert_asm("mov eax, 1", 0);
    convert_asm("mov ebx, 1", 0);
    convert_asm("mov rsi, 0x00000001", 0);
    convert_asm("mov edx, 10", 0);
    convert_asm("syscall", 0);
    convert_asm("mov eax, 1", 0);
    convert_asm("mov ebx, 1", 0);
    convert_asm("mov rsi, 0x00000002", 0);
    convert_asm("mov edx, 10", 0);
    convert_asm("syscall", 0);
    convert_asm("mov eax, 60", 0);
    convert_asm("mov ebx, 0", 0);
    convert_asm("syscall", 0);
    OpCodes[OpCodeCount++] = (opc){
        .code = to_heap((u8 []){0xC3}, 1),
        .bytes = 1,
        .needs_ptr = 0
    };
    OpCodes[OpCodeCount++] = (opc){
        .code = to_heap((u8 []){0xFF, 0x00, 0xFF}, 3),
        .bytes = 3,
        .needs_ptr = 0
    };
    println("OpCodes: ");
    u8 final_executable[65535];
    int idx = 0;
    u8 count = 0;
    for(int i = 0; i < OpCodeCount; i++)
    {
        if(i == 3 && i == OpCodeCount - 1)
            continue;

        char byte[3];
        _printf("Bytes: %d -> ", (void *)&OpCodes[i].bytes);
        for(int c = 0; c < OpCodes[i].bytes; c++)
        {
            if(i > 6) count++;
            final_executable[idx++] = OpCodes[i].code[c];
            byte_to_hex(OpCodes[i].code[c], byte);
            _printf(c == OpCodes[i].bytes - 1 ? "%s" : "%s, ", byte);
        }
        println(NULL);
    }

    //O_WRONLY | O_CREAT | O_TRUNC
    fd_t file = open_file("fag.bin", 0, _O_WRONLY | _O_CREAT | _O_TRUNC);
    u8 hello_len = 6;
    u8 test_len = 5;

    file_write(file, final_executable, idx);
    file_write(file, &hello_len, sizeof(u8));
    file_write(file, &BLACKSPACE, sizeof(u8));
    file_write(file, "Hello\n", hello_len);
    file_write(file, &NULL_TERMINATOR, sizeof(u8));
    file_write(file, &test_len, sizeof(u8));
    file_write(file, &BLACKSPACE, sizeof(u8));
    file_write(file, "TEST\n", test_len);
    file_write(file, &NULL_TERMINATOR, sizeof(u8));
    file_close(file);
    return 0;
}