#include <libbase.h>

extern unsigned char _binary_fag_bin_start[];
extern unsigned char _binary_fag_bin_end[];

extern int _binary_fag_bin_size;

int entry() {
    size_t size = (size_t)&_binary_fag_bin_size; // or _binary_fag_bin_end - _binary_fag_bin_start
    char hex[3];

    for (size_t i = 0; i < size; i++) {
        byte_to_hex(_binary_fag_bin_start[i], hex);
        printf("%s%s", hex, (i == size - 1) ? "\n" : ", ");
    }
    return 0;
}
