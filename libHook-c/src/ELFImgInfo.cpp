
#include <type/ELFImgInfo.h>


scaler::ELFImgInfo::ELFImgInfo(void *pltStartAddr, void *pltSecStartAddr) : pltSecStartAddr(pltSecStartAddr),
                                                                            pltStartAddr(pltStartAddr) {

}

scaler::ELFImgInfo::ELFImgInfo() {

}
