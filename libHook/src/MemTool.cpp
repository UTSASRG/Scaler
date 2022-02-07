#include <util/tool/MemTool.h>


#ifdef __linux

#include <elf.h>
#include <link.h>
#include <cmath>
#include <sstream>
#include <exceptions//ScalerException.h>
#include <sys/mman.h>
#include <unistd.h>

namespace scaler {

    void *MemoryTool::searchBinInMemory(void *segPtrInFile, ssize_t firstEntrySize,
                                        const std::vector<PMEntry_Linux> &segments, void *boundStartAddr,
                                        void *boundEndAddr) {
        void *rltAddr = nullptr;

        for (int i = 0; i < segments.size(); ++i) {
            if (boundStartAddr <= segments[i].addrStart && segments[i].addrEnd <= boundEndAddr) {
                rltAddr = binCodeSearch(segments[i].addrStart, segments[i].length, segPtrInFile, firstEntrySize);
                if (rltAddr)
                    break;
            }
        }
        return rltAddr;
    }

    void *MemoryTool::binCodeSearch(void *target, ssize_t targetSize, void *keyword, ssize_t keywordSize) {
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

    MemoryTool *MemoryTool::getInst() {
        if (MemoryTool::instance == nullptr) {
            MemoryTool::instance = new MemoryTool();
            if (!MemoryTool::instance) {
                fatalError("Cannot allocate memory for MemoryTool");
            }
        }
        return MemoryTool::instance;
    }

    //Initialize instance
    MemoryTool *MemoryTool::instance = nullptr;

    MemoryTool::~MemoryTool() = default;

    MemoryTool::MemoryTool() {

    }
}
#endif