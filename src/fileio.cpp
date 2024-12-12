#include "ff.h"
#include "stdint.h"
#include "string.h"

void* fileio_open(const char* filename) {
    FIL *fil = new FIL();
    FRESULT fr = f_open((FIL*)fil, filename, FA_READ);
    (void)fr;
    return (void *)fil;
}

void fileio_close(void* fhandle) {
    f_close((FIL*)fhandle);
}

size_t fileio_read(void* fhandle, void *buf, size_t len) {
    unsigned int bytes_read;
    FRESULT fr = f_read((FIL*)fhandle, buf, len, &bytes_read);
    (void)fr;
    return bytes_read;
}

int fileio_getc(void* fhandle) {
    unsigned int  bytes_read;
    char c;
    FRESULT fr = f_read((FIL*)fhandle, (void*)&c, 1, &bytes_read);
    (void)fr;
    (void)bytes_read;
    return c;
}

size_t fileio_tell(void* fhandle) {
    return f_tell((FIL*)fhandle);
}

size_t fileio_seek(void* fhandle, size_t pos) {
    f_lseek((FIL*)fhandle, pos);
    return pos;
}