#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
 // loads a bin file
int main() {
    int fd = open("test.bin", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct stat st;
    fstat(fd, &st);
    size_t size = st.st_size;

    /* Allocate RWX memory */
    void *mem = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (mem == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    /* Load shellcode */
    read(fd, mem, size);
    close(fd);

    /* Jump to code */
    void (*entry)() = mem;
    entry();

    return 0;
}
