/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/

#ifndef SCALER_MEMORYTOOL_H
#define SCALER_MEMORYTOOL_H

#include <cstdio>
#include <cstdint>
#include <cassert>
#include "util/tool/Logging.h"
//Address is aligned to page_size. Map addr to the starting of page boundary
//mprotect requires address to be a page boundary
//eg: If page size is 1024 and 1025's page boundary will be 1025/1024*1024=1024
#define GET_PAGE_BOUND(addr, page_size) (Elf64_Addr *) ((ssize_t) (addr) / page_size * page_size)

namespace scaler {
    void *memSearch(void *target, ssize_t targetSize, void *keyword, ssize_t keywordSize);

    bool adjustMemPerm(void *startPtr, void *endPtr, int prem);

    inline uint8_t *autoAddBaseAddr(uint8_t *targetAddr, uint8_t *baseAddr, uint8_t *startAddr, uint8_t *endAddr) {
        uint8_t *rlt = baseAddr + (uint64_t) targetAddr;
        //INFO_LOGS("baseAddr=%p, targetAddr=%p,startAddr=%p, endAddr=%p",baseAddr,targetAddr,startAddr,endAddr);
        if (rlt < startAddr || rlt > endAddr) {
            //INFO_LOG("rlt falls out of range, targetAddr must be in range");
            //rlt falls out of range, targetAddr must be in range
            assert(startAddr <= targetAddr && targetAddr <=endAddr);
            //Return targetAddress instead
            rlt = targetAddr;
        }
        //rlt falls in range
        return rlt;
    }
}
#endif //SCALER_MEMORYTOOL_H
