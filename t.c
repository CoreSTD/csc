#include <libbase.h>

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
            // OpCodes[OpCodeCount++] = (opc){
            //     .code = N <= 0x7FFFFFFF ? mov_imm32_reg(reg, value) : mov_imm64_reg(reg, value),
            //     .needs_ptr = 0,
            //     .bytes = 3
            // };
        }
        
        // Pushing a number to register (Must detect max number for imm32 or imm64)
        if(is_number(args[2]))
        {
            mov reg = reg_to_type(args[1]);
            char buff[100];
            byte_to_hex(reg, buff);
            _printf("Register: %s | ", buff);
            u64 value = str_to_int(args[2]); // This needs to be checked for max number of i32
            _printf("Num: %d\n", (void *)&value);
            u8 *c0de = ARCH_MODE == x86 ? c0de = mov_imm32_reg(reg, value) : mov_imm64_reg(reg, value);
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

    // lea reg, [VARIABLE]
    if(str_startswith(q, "lea"))
    {
        if(argc < 3)
            lb_panic("Invalid opcode...!");

        // Variable detected ()
        if(str_startswith(args[2], "[") && str_endswith(args[2], "]"))
        {
            
        }
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

    return (opc){0};
}

i8 entry(int argc, string argv[])
{
    if(argc > 1 && array_contains_str((array)argv, "--64"))
        ARCH_MODE = x86_64;
    else
        ARCH_MODE = x86;

    int n = 4;
    // OpCodes[OpCodeCount++] = (opc){
    //     .code = to_heap((u8 []){'H', 'E', 'L', 'L', 'O'}, sizeof(u8 *) * 4),
    //     .bytes = 4,
    //     .needs_ptr = 5
    // };
    OpCodes[OpCodeCount++] = (opc){
        .code = to_heap((u8 []){0x01, 0x02, 0x03, 0x04}, sizeof(u8 *) * 4),
        .bytes = 4,
        .needs_ptr = 0
    };
    convert_asm("xor rdi, rdi", 0);
    convert_asm("mov rax, 1", 0);
    convert_asm("mov rdi, 1", 0);
    convert_asm("mov rsi, 5", 0);
    convert_asm("mov rdx, 10", 0);
    convert_asm("syscall", 0);
    convert_asm("mov rax, 60", 0);
    convert_asm("mov rdi, 0", 0);
    convert_asm("syscall", 0);
    println("OpCodes: ");
    u8 final_executable[65535];
    int idx = 0;
    for(int i = 0; i < OpCodeCount; i++)
    {
        char byte[3] = {0};
        _printf("Bytes: %d -> ", (void *)&OpCodes[i].bytes);
        for(int c = 0; c < OpCodes[i].bytes; c++)
        {
            final_executable[idx++] = OpCodes[i].code[c];
            byte_to_hex(OpCodes[i].code[c], byte);
            _printf(c == OpCodes[i].bytes - 1 ? "%s" : "%s, ", byte);
        }
        println(NULL);
    }

    //O_WRONLY | O_CREAT | O_TRUNC
    fd_t file = open_file("dick.bin", 0, _O_WRONLY | _O_CREAT | _O_TRUNC);
    file_write(file, "Hello", 5);
    file_write(file, final_executable, idx);
    file_close(file);
    return 0;
}