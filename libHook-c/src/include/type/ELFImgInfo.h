#ifndef SCALER_ELFIMGINFO_H
#define SCALER_ELFIMGINFO_H

#include <util/datastructure/Vector.h>
#include "ExtSymInfo.h"

namespace scaler {
/**
* ELF image (ELF file in memory) information.
*/
    class ELFImgInfo {
    public:
        void *pltStartAddr = nullptr;                   //The starting address of the PLT table
        void *pltSecStartAddr = nullptr;                //The starting address of the PLT.SEC table

        //The first index of hooked symbol inside hookedExtSymbol;
        ssize_t symbolIndexInHookedExtSymbol;
        //The number of hooked symbol in this elf image;
        ssize_t hookedSymbolSize;
        Vector<ExtSymInfo> hookedInfo;
        ELFImgInfo();
        ELFImgInfo(void* pltStartAddr,void* pltSecStartAddr);
    };
}
#endif //SCALER_ELFIMGINFO_H
