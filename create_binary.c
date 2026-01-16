#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <elf.h>

#define BASE_ADDR 0x400000

int main(void)
{
    FILE *f = fopen("hello_elf", "wb");
    if (!f) return 1;

    /* ---------------- ELF HEADER ---------------- */

    Elf64_Ehdr eh = {0};

    memcpy(eh.e_ident, ELFMAG, SELFMAG);
    eh.e_ident[EI_CLASS] = ELFCLASS64;
    eh.e_ident[EI_DATA]  = ELFDATA2LSB;
    eh.e_ident[EI_VERSION] = EV_CURRENT;
    eh.e_ident[EI_OSABI] = ELFOSABI_SYSV;

    eh.e_type      = ET_EXEC;
    eh.e_machine   = EM_X86_64;
    eh.e_version   = EV_CURRENT;
    eh.e_entry     = BASE_ADDR + sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
    eh.e_phoff     = sizeof(Elf64_Ehdr);
    eh.e_ehsize    = sizeof(Elf64_Ehdr);
    eh.e_phentsize = sizeof(Elf64_Phdr);
    eh.e_phnum     = 1;

    fwrite(&eh, sizeof(eh), 1, f);

    /* ---------------- PROGRAM HEADER ---------------- */

    Elf64_Phdr ph = {0};

    ph.p_type   = PT_LOAD;
    ph.p_offset = 0;
    ph.p_vaddr  = BASE_ADDR;
    ph.p_paddr  = BASE_ADDR;
    ph.p_flags  = PF_R | PF_X;
    ph.p_align  = 0x1000;

    long ph_pos = ftell(f);
    fwrite(&ph, sizeof(ph), 1, f);

    /* ---------------- MACHINE CODE ---------------- */

    uint8_t code[] = {
        0x48,0xc7,0xc0,0x01,0x00,0x00,0x00,        // mov rax,1
        0x48,0xc7,0xc7,0x01,0x00,0x00,0x00,        // mov rdi,1
        0x48,0x8d,0x35,0x32,0x00,0x00,0x00,        // lea rsi,[rip+14]
        0x48,0xc7,0xc2,0x14,0x00,0x00,0x00,        // mov rdx,13
        0x0f,0x05,                                // syscall
        0x48,0xc7,0xc0,0x01,0x00,0x00,0x00,
        0x48,0xc7,0xc7,0x01,0x00,0x00,0x00,
        0x48,0x8d,0x35,0x27,0x00,0x00,0x00,
        0x48,0xc7,0xc2,0x0b,0x00,0x00,0x00,
        0x0f,0x05,
        0x48,0xc7,0xc0,0x3c,0x00,0x00,0x00,        // mov rax,60
        0x48,0x31,0xff,                            // xor rdi,rdi
        0x0f,0x05                                 // syscall
    };

    long code_off = ftell(f);
    fwrite(code, sizeof(code), 1, f);

    /* ---------------- STRING DATA ---------------- */

    const char msg[] = "Hello, world\n";
    fwrite(msg, sizeof(msg) - 1, 1, f);

    const char gay[] = "With clib+\n";
    fwrite(gay, sizeof(gay) - 1, 1, f);

    /* ---------------- FIX PROGRAM HEADER SIZES ---------------- */

    long end = ftell(f);
    ph.p_filesz = end;
    ph.p_memsz  = end;

    fseek(f, ph_pos, SEEK_SET);
    fwrite(&ph, sizeof(ph), 1, f);

    fclose(f);

    printf("Created ELF executable: hello_elf\n");
    return 0;
}
