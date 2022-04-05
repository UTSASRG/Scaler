#include <util/tool/AddrFileIdMapping.h>

namespace scaler {


    bool calcBestBucketSize(Array<PltRange> &pltRangeArr, ssize_t &optimalBucketSize, uint64_t &retBaseAddr) {
        //todo: Handle corner case (When there is entry, throw error, when there is only entry, the size of that entry is the optimal split)
        //todo: Do not forget to adjust pltEntry offset. This is should not be the starting address of the plt entry.
        //Endpoints in this array need to be re-checked.

        assert(pltRangeArr.getSize() > 2);
        Array<Blank> blanksArr;
        /**
         * At first, insert the id of all executable segments to invalidatedStack. Because everything needs to be checked.
         * Assume pltRangeArr is sorted from low addr to high
         */
        retBaseAddr = 0;
        for (int i = 0; i < pltRangeArr.getSize() - 1; ++i) {
            if (!retBaseAddr)
                retBaseAddr = pltRangeArr[i].addrStart;
            Blank *curBlank = blanksArr.pushBack();
            curBlank->addrStart = pltRangeArr[i].addrEnd - retBaseAddr;
            curBlank->addrEnd = pltRangeArr[i + 1].addrStart - retBaseAddr;
        }

        /**
         * Initialize optimal split size to the largest possible value. The rest will keep decreasing
         */
        optimalBucketSize = blanksArr[0].addrEnd;

        bool hasInavlidSeg = false;
        while (true) {
            /**
             * 1. Find one invalid segment
             * Check every entry in invalidatedStack.
             * If the split satisfy, then this entry should be removed by swapping arr[i] with arr[length-1].
             */
            hasInavlidSeg = false;
            for (ssize_t i = 0; i < blanksArr.getSize(); ++i) {
                DBG_LOGS("%lu-%lu", blanksArr[i].addrStart, blanksArr[i].addrEnd);
                if (!isBlankValid(reinterpret_cast<uint64_t>(blanksArr[i].addrStart),
                                  reinterpret_cast<uint64_t>(blanksArr[i].addrEnd), optimalBucketSize)) {
                    hasInavlidSeg = true;
                    break;
                }
            }
            if (!hasInavlidSeg)
                break;

            /**
             * 2. Shrink optimalSplit size
             */
            optimalBucketSize >>= 1;
            DBG_LOGS("optimalSplitSize=%zd", optimalBucketSize);
        }

        return true;
    }

    bool isBlankValid(uint64_t blankStart, uint64_t blankEnd, ssize_t &optimalSplitSize) {
        assert(ssize_t(blankStart / optimalSplitSize) <= ssize_t(blankEnd / optimalSplitSize));
        return ssize_t(blankStart / optimalSplitSize) != ssize_t(blankEnd / optimalSplitSize);
    }


}