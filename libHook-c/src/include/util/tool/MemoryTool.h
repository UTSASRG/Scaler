//
// Created by st on 7/6/21.
//

#ifndef SCALER_MEMORYTOOL_H
#define SCALER_MEMORYTOOL_H

#include <cstdio>
#include <cstdint>


//Address is aligned to page_size. Map addr to the starting of page boundary
//mprotect requires address to be a page boundary
//eg: If page size is 1024 and 1025's page boundary will be 1025/1024*1024=1024
#define GET_PAGE_BOUND(addr, page_size) (Elf64_Addr *) ((ssize_t) (addr) / page_size * page_size)

namespace scaler {
    void *memSearch(void *target, ssize_t targetSize, void *keyword, ssize_t keywordSize);

    bool adjustMemPerm(void *startPtr, void *endPtr, int prem);

    inline uint8_t *autoAddBaseAddr(uint8_t *targetAddr, uint8_t *baseAddr, uint8_t *startAddr, uint8_t *endAddr) {
        uint8_t *rlt = baseAddr + (uint64_t) targetAddr;
        if (rlt < startAddr && endAddr > rlt) {
            //Rlt not in the desired range
            rlt = targetAddr;
        } else if (startAddr <= targetAddr &&
                   targetAddr <= endAddr) {
            rlt = targetAddr;
        }
        return rlt;
    }
}
#endif //SCALER_MEMORYTOOL_H
