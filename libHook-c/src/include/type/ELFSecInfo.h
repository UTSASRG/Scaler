#ifndef ELFSECINFO_H
#define ELFSECINFO_H

#include<ctype.h>
#include<cstdio>

struct ELFSecInfo {
    uint8_t *startAddr;
    Elf64_Word size;
    Elf64_Word entrySize;
};
#endif