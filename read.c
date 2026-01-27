#include <libbase.h>

int entry()
{
    long ret = __syscall__(0, _HEAP_PAGE_, 0x01 | 0x02 | 0x04, 0x02 | 0x20, -1, 0, _SYS_MMAP);
    if(ret < 0)
        lb_panic("failed to create execution buffer!");

    heap_t heap = (heap_t)ret;
    fd_t file = open_file("dick.bin", 0, 0);
    int sz = file_content_size(file);

    __syscall__(file, (long)heap, sz, -1, -1, -1, _SYS_READ);
    char BUFFERS[10][1024];
    int idx = 0, len = 0;
    int capture_code;
    for(int i = 0; i < sz; i++)
    {
        if(((char *)heap)[i] == 0x01 && ((char *)heap)[i + 1] == 0x02 &&
         ((char *)heap)[i + 2] == 0x03 && ((char *)heap)[i + 3] == 0x04)
            capture_code = 1;
        
        if(capture_code) {
            char buff[3];
            byte_to_hex(((char *)heap)[i], buff);
            _printf("%s, ", buff);
            
            continue;
        }

        BUFFERS[idx][len++] = ((char *)heap)[i];
        if(((char *)heap)[i] == '\0')
            idx++;
    }

    // void *(*dick)() = heap;
    // dick();
    return 0;
}