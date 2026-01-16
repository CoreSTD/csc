#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <elf.h>

#define DIE(x) do { perror(x); exit(1); } while (0)

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <input-elf> <output-elf>\n", argv[0]);
        return 1;
    }

    /* ---------- read input ---------- */

    FILE *in = fopen(argv[1], "rb");
    if (!in) DIE("fopen input");

    fseek(in, 0, SEEK_END);
    size_t insz = ftell(in);
    rewind(in);

    uint8_t *inbuf = malloc(insz);
    if (!inbuf) DIE("malloc");

    fread(inbuf, 1, insz, in);
    fclose(in);

    Elf64_Ehdr *eh = (Elf64_Ehdr *)inbuf;

    if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 ||
        eh->e_ident[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "Not an ELF64 file\n");
        return 1;
    }

    Elf64_Phdr *ph = (Elf64_Phdr *)(inbuf + eh->e_phoff);

    /* ---------- write minimal ELF ---------- */

    FILE *out = fopen(argv[2], "wb");
    if (!out) DIE("fopen output");

    Elf64_Ehdr new_eh = *eh;

    /* strip section headers */
    new_eh.e_shoff = 0;
    new_eh.e_shnum = 0;
    new_eh.e_shentsize = 0;
    new_eh.e_shstrndx = SHN_UNDEF;

    /* place PHDRs immediately after ELF header */
    new_eh.e_phoff = sizeof(Elf64_Ehdr);
    new_eh.e_phentsize = sizeof(Elf64_Phdr);

    fwrite(&new_eh, sizeof(new_eh), 1, out);

    long phdr_off = ftell(out);
    fwrite(ph, sizeof(Elf64_Phdr), eh->e_phnum, out);

    /* ---------- copy loadable segments ---------- */

    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD)
            continue;

        long cur = ftell(out);

        /* align file offset */
        long align = ph[i].p_align ? ph[i].p_align : 1;
        long pad = (align - (cur % align)) % align;
        for (long p = 0; p < pad; p++)
            fputc(0, out);

        long new_off = ftell(out);

        /* copy segment data */
        fwrite(inbuf + ph[i].p_offset, 1, ph[i].p_filesz, out);

        /* update program header */
        ph[i].p_offset = new_off;
    }

    /* ---------- rewrite updated PHDRs ---------- */

    long end = ftell(out);
    fseek(out, phdr_off, SEEK_SET);
    fwrite(ph, sizeof(Elf64_Phdr), eh->e_phnum, out);

    fclose(out);
    free(inbuf);

    printf("Stripped ELF written to %s\n", argv[2]);
    return 0;
}
