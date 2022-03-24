//
// Created by st on 7/6/21.
//

#ifndef SCALER_MEMORYTOOL_H
#define SCALER_MEMORYTOOL_H

#include <cstdio>

//Address is aligned to page_size. Map addr to the starting of page boundary
//mprotect requires address to be a page boundary
//eg: If page size is 1024 and 1025's page boundary will be 1025/1024*1024=1024
#define GET_PAGE_BOUND(addr, page_size) (Elf64_Addr *) ((ssize_t) (addr) / page_size * page_size)

namespace scaler {
    void *memSearch(void *target, ssize_t targetSize, void *keyword, ssize_t keywordSize);

    bool adjustMemPerm(void *startPtr, void *endPtr, int prem);

}

#endif //SCALER_MEMORYTOOL_H
