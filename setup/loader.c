#include <libbase.h>

typedef enum {
    x = 0,
    x86 = 0x32,
    x86_64 = 0x64
} arch_t;

bool is_file_lb(u8 *binary)
{ return (binary[0] == 'L' && binary[1] == 'B'); }

bool is_file_x86(u8 arch)
{ return (arch == 0x32 ? 0 : (arch == 0x64 ? 1 : -1)); }

bool is_file_executable(u8 *type)
{
    return (type[0] == 'E' && type[1] == 'X' && type[2] == 'E') ? 0 : 
            (type[0] == 'D' && type[1] == 'Y' && type[2] == 'M') ? 1 : -1; 
}

typedef struct 
{
    string      filename;
    fd_t        file;
    i32         size;
    u8          *buffer;
    u8          FILE_TYPE[2];
    arch_t      ARCH;
    u8          BUFFER_SEGMENT;
    u8          *OPCODE;
    i32         CODE_COUNT;
    i32         BUFFER_SEG;
    u8          **STRINGS;
    i32         *OFFSETS;
    map_t       memory_map;
} binary_t;

#define LB_TYPE_OFFSET 2
#define ARCH_OFFSET 5
#define CODE_SEGMENT_OFFSET 6

binary_t *init_lb(string filename)
{
    if(!filename)
        return NULL;

    binary_t *b = allocate(0, sizeof(binary_t));

    b->filename = filename;
    b->file = open_file(filename, 0, 0);
    if(!b->file)
        fsl_panic("Unable to open binary file...!");

    b->size = file_content_size(b->file);
    if(b->size <= 0)
        fsl_panic("Unable to reach EOF...!");

    b->buffer = allocate(0, b->size);
    __syscall__(b->file, (long)b->buffer, b->size, -1, -1, -1, _SYS_READ);
    file_close(b->file);
    return b;
}

fn validate_file(binary_t *b) {
    int lb, type, arch;
    if(!(lb = is_file_lb(b->buffer)))
        println("Not a LB FILE!");

    _printf("Is LB binary: %s\n", lb ? "Yes" : "No");
    
    if((type = is_file_executable(b->buffer + LB_TYPE_OFFSET)) == -1)
        fsl_panic("Unable to detect lb file type");

    _printf("File Type: %s\n", type == 0 ? "Executable" : type ? "Shared Lib" : "None");

    if(is_file_x86(b->ARCH) > -1)
        _printf("Is a 32-bit file\n", NULL);
}

fn parse_file(binary_t *b)
{
    long ret = __syscall__(0, _HEAP_PAGE_, PROT_READ | PROT_EXEC, 0x02 | 0x20 | 0x40 , -1, 0, _SYS_MMAP);
    if(ret < 0)
        fsl_panic("failed to create execution buffer!");

    // if ((uintptr_t)ret > 0xFFFFFFFF)
    //     fsl_panic("MAP_32BIT did not give 32-bit address");

    b->OPCODE = (u8 *)ret;
    b->CODE_COUNT = 0;
    __syscall__((long)b->OPCODE, _HEAP_PAGE_, PROT_READ | PROT_WRITE | PROT_EXEC, 0, 0, 0, _SYS_MPROTECT);
    for(int i = 6; i < b->size; i++)
    {
        if(b->buffer[i] == 0xFF && b->buffer[i + 1] == 0x00 && b->buffer[i + 2] == 0xFF) {
            b->BUFFER_SEGMENT = i + 3;
            break;
        }
        
        if(__FSL_DEBUG__) {
            char buff[3];
            byte_to_hex(((char *)b->buffer)[i], buff);
            _printf("%s, ", buff);
        }

        b->OPCODE[b->CODE_COUNT++] = b->buffer[i];
    }
}

fn parse_buffers(binary_t *b)
{
    b->STRINGS = allocate(0, sizeof(u8 *));
    u8 nbuff[1024];
    int bidx = 0, offsets = 0;
    for(int i = b->BUFFER_SEGMENT, n = 0; i < b->size; i++)
    {
        if(b->buffer[i] == 0x00 && b->buffer[i + 1] == 0xFF)
            { i+= 2; continue; }

        if(b->buffer[i] == 0x00) {
            if(__FSL_DEBUG__) _printf(" -> %s\n", nbuff)
            b->STRINGS[bidx++] = to_heap(nbuff, n);
            b->STRINGS = reallocate(b->STRINGS, sizeof(u8 *) * (bidx + 1));
            b->STRINGS[bidx] = NULL;
            n = 0;
            i += 2;
            continue;
        }

        if(is_ascii(b->buffer[i]) && b->buffer[i + 1] == 0xFF) {
            if(__FSL_DEBUG__) println("\nSIZE DETECTED");
            i++;
            continue;
        }

        nbuff[n++] = b->buffer[i];
        nbuff[n] = '\0';

        if(__FSL_DEBUG__) {
            char buff[3];
            byte_to_hex(((char *)b->buffer)[i], buff);
            
            _printf(b->buffer[i + 1] != 0x00 ? "%s, " : "%s", buff);
        }
    }
}

int entry(int argc, string argv[])
{
    binary_t *b = init_lb("fag.bin");
    if(b->size == 0)
        fsl_panic("No bytes in file to read...!");

    validate_file(b);
    parse_file(b);
    parse_buffers(b);
    
    uintptr_t page_start = (uintptr_t)b->OPCODE & ~(_HEAP_PAGE_ - 1);

    _printf("Buffer Start: %d\n", (void *)&b->BUFFER_SEG);
    for(int i = 0 ; i < 2; i++)
        println(b->STRINGS[i]);
    
    println("CODE SEGMENT");
    char byte[3];
    for(int i = 0, nl = 0, strs = 0; i < b->CODE_COUNT; i++)
    {
        if(i == nl + 5) {
            println(NULL);
            nl += 5;
        }

        if(i <= b->CODE_COUNT - 8 && b->OPCODE[i] == 0x69 && b->OPCODE[i+1] == 0x69 && b->OPCODE[i+2] == 0x69 && b->OPCODE[i+3] == 0x69)
        {
            uintptr_t addr = (uintptr_t)b->STRINGS[strs++];
            _printf("String: %s\n", b->STRINGS[strs - 1]);

            size_t placeholder_offset = i;
            for(size_t j = 0; j < sizeof(uintptr_t); j++)
                b->OPCODE[placeholder_offset + j] = (addr >> (j * 8)) & 0xFF;

            _printf("Replacing pointer: %p\n", (void*)addr);

            i += sizeof(uintptr_t) - 1;
            continue;
        }

        char byte[3];
        byte_to_hex(b->OPCODE[i], byte);
        _printf(i == nl - 5 ? "%s" : "%s, ", byte);
    }

    __syscall__(page_start, _HEAP_PAGE_, PROT_READ | PROT_EXEC, -1, -1, -1, _SYS_MPROTECT);
    println("EXECUTING");
    void *(*test)() = (void *)b->OPCODE;
    test();
    return 0;
}