#ifndef SCALER_ELFIMGINFO_H
#define SCALER_ELFIMGINFO_H

#include <util/datastructure/Vector.h>
#include "ExtSymInfo.h"

namespace scaler {
/**
* ELF image (ELF file in memory) information.
*/
    struct ELFImgInfo {
        uint8_t *pltStartAddr;
        uint8_t *pltSecStartAddr;
        uint8_t *gotStartAddr;
        int64_t firstSymIndex;
        bool valid = false;
    };
}
#endif //SCALER_ELFIMGINFO_H
