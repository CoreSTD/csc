#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <elf.h>

#define DIE(msg) do { perror(msg); exit(1); } while (0)

static void dump_bytes(const uint8_t *buf, size_t size, uint64_t base)
{
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0)
            printf("%08lx: ", base + i);
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    if (size % 16)
        printf("\n");
}

static const char *elf_class(uint8_t c)
{
    return c == ELFCLASS64 ? "ELF64" :
           c == ELFCLASS32 ? "ELF32" : "UNKNOWN";
}

static const char *elf_endian(uint8_t e)
{
    return e == ELFDATA2LSB ? "Little Endian" :
           e == ELFDATA2MSB ? "Big Endian" : "UNKNOWN";
}

/* ---------------- ELF64 ---------------- */

static void parse_elf64(uint8_t *data, size_t size)
{
    Elf64_Ehdr *eh = (Elf64_Ehdr *)data;

    printf("\n== ELF HEADER ==\n");
    printf("Class:     %s\n", elf_class(eh->e_ident[EI_CLASS]));
    printf("Endian:    %s\n", elf_endian(eh->e_ident[EI_DATA]));
    printf("Type:      %u\n", eh->e_type);
    printf("Machine:   %u\n", eh->e_machine);
    printf("Entry:     0x%lx\n", eh->e_entry);
    printf("PH off:    0x%lx\n", eh->e_phoff);
    printf("SH off:    0x%lx\n", eh->e_shoff);

    /* Program headers */
    printf("\n== PROGRAM HEADERS ==\n");
    Elf64_Phdr *ph = (Elf64_Phdr *)(data + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; i++) {
        printf("[%d] Type=%u Off=0x%lx Vaddr=0x%lx Filesz=0x%lx Memsz=0x%lx Flags=0x%x\n",
               i, ph[i].p_type, ph[i].p_offset,
               ph[i].p_vaddr, ph[i].p_filesz,
               ph[i].p_memsz, ph[i].p_flags);
    }

    /* Section headers */
    printf("\n== SECTION HEADERS ==\n");
    Elf64_Shdr *sh = (Elf64_Shdr *)(data + eh->e_shoff);
    const char *shstr = (const char *)(data + sh[eh->e_shstrndx].sh_offset);

    for (int i = 0; i < eh->e_shnum; i++) {
        printf("[%2d] %-16s Off=0x%08lx Size=0x%08lx Type=%u Flags=0x%lx\n",
               i,
               shstr + sh[i].sh_name,
               sh[i].sh_offset,
               sh[i].sh_size,
               sh[i].sh_type,
               sh[i].sh_flags);
    }

    /* Dump executable sections */
    printf("\n== MACHINE CODE (EXECUTABLE SECTIONS) ==\n");
    for (int i = 0; i < eh->e_shnum; i++) {
        if (sh[i].sh_flags & SHF_EXECINSTR) {
            printf("\n-- %s --\n", shstr + sh[i].sh_name);
            dump_bytes(data + sh[i].sh_offset,
                       sh[i].sh_size,
                       sh[i].sh_addr);
        }
    }

    /* Symbols */
    printf("\n== SYMBOL TABLES ==\n");
    for (int i = 0; i < eh->e_shnum; i++) {
        if (sh[i].sh_type == SHT_SYMTAB || sh[i].sh_type == SHT_DYNSYM) {
            Elf64_Sym *sym = (Elf64_Sym *)(data + sh[i].sh_offset);
            int count = sh[i].sh_size / sizeof(Elf64_Sym);
            const char *strtab = (const char *)(data + sh[sh[i].sh_link].sh_offset);

            printf("\n-- %s --\n", shstr + sh[i].sh_name);
            for (int j = 0; j < count; j++) {
                printf("%016lx %5lu %-20s\n",
                       sym[j].st_value,
                       sym[j].st_size,
                       strtab + sym[j].st_name);
            }
        }
    }
}

/* ---------------- ELF32 ---------------- */

static void parse_elf32(uint8_t *data, size_t size)
{
    Elf32_Ehdr *eh = (Elf32_Ehdr *)data;

    printf("\n== ELF HEADER ==\n");
    printf("Class:     ELF32\n");
    printf("Entry:     0x%x\n", eh->e_entry);

    Elf32_Shdr *sh = (Elf32_Shdr *)(data + eh->e_shoff);
    const char *shstr = (const char *)(data + sh[eh->e_shstrndx].sh_offset);

    printf("\n== SECTION HEADERS ==\n");
    for (int i = 0; i < eh->e_shnum; i++) {
        printf("[%2d] %-16s Off=0x%08x Size=0x%08x Flags=0x%x\n",
               i,
               shstr + sh[i].sh_name,
               sh[i].sh_offset,
               sh[i].sh_size,
               sh[i].sh_flags);
    }

    printf("\n== MACHINE CODE ==\n");
    for (int i = 0; i < eh->e_shnum; i++) {
        if (sh[i].sh_flags & SHF_EXECINSTR) {
            printf("\n-- %s --\n", shstr + sh[i].sh_name);
            dump_bytes(data + sh[i].sh_offset,
                       sh[i].sh_size,
                       sh[i].sh_addr);
        }
    }
}

/* ---------------- MAIN ---------------- */

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <binary>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) DIE("fopen");

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    uint8_t *data = malloc(size);
    if (!data) DIE("malloc");

    fread(data, 1, size, f);
    fclose(f);

    if (memcmp(data, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not an ELF file\n");
        return 1;
    }

    uint8_t cls = data[EI_CLASS];
    if (cls == ELFCLASS64)
        parse_elf64(data, size);
    else if (cls == ELFCLASS32)
        parse_elf32(data, size);
    else
        fprintf(stderr, "Unknown ELF class\n");

    free(data);
    return 0;
}
