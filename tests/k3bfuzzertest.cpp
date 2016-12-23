#include <QDebug>

#include "tools/libisofs/isofs.h"

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) 
{
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << *argc;
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << *argv[0];
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) 
{
    char *d = (char *)malloc(1024);
    memset(d, 0, 1024);
    strncpy(d, "hello", 5);
    str_nappend(&d, (char *)Data, Size);
    free(d);
    return 0;
}
