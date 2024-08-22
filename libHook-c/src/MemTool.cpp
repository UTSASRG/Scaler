/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <util/tool/MemoryTool.h>


#ifdef __linux

#include <elf.h>
#include <link.h>
#include <cmath>
#include <sstream>
#include <exceptions//ScalerException.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <csignal>

namespace scaler {

    void *memSearch(void *target, ssize_t targetSize, void *keyword, ssize_t keywordSize) {
        //Convert it to uint8* so that we can perform arithmetic operation on those pointers
        uint8_t *kwd = static_cast<uint8_t *>(keyword);
        uint8_t *tgt = static_cast<uint8_t *>(target);

        int i = 0, j = 0; //i is the index in target and j is the index in keyword
        uint8_t *beg = nullptr; //Markes the begging of the match

        while (i < targetSize && j < keywordSize) {
            if (tgt[i] == kwd[j]) {
                if (beg == nullptr) {
                    //First match. It's a potential starting position.
                    beg = tgt + i;
                }
                ++j;
            } else {
                //If tgt[i] != kwd[j] it means this is not the correct keyword. Reset beg and j.
                beg = nullptr;
                j = 0;
            }
            ++i;
        }
        // If j==keywordSize it means the previous loop exit because of this. Then it means a match if found.
        return j == keywordSize ? beg : nullptr;
    }

    bool adjustMemPerm(void *startPtr, void *endPtr, int prem) {
        //Get page size
        ssize_t pageSize = sysconf(_SC_PAGESIZE);
        //Get Page Bound
        void *startPtrBound = GET_PAGE_BOUND(startPtr, pageSize);
        void *endPtrBound = endPtr;
        if (startPtrBound == endPtrBound)
            endPtrBound = (uint8_t *) startPtrBound + pageSize;

        //todo:(uint8_t *) endPtrBound - (uint8_t  *) startPtrBound,
        ssize_t memoryLength =
                (ceil(((uint8_t *) endPtrBound - (uint8_t *) startPtrBound) / (double) pageSize)) * pageSize;
        //DBG_LOGS("Real addr from:%p to:%p", startPtrBound, endPtrBound);
        if (mprotect(startPtrBound, memoryLength, prem) != 0) {
            ERR_LOGS("Could not change the process memory permission at %p-%p because: %s", startPtrBound, endPtrBound,
                     strerror(errno));
            //signal(SIGINT, 0);
            return false;
        }
        return true;
    }


}
#endif