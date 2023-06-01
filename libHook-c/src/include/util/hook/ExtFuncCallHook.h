#ifndef SCALER_EXTFUNCCALLHOOK_H
#define SCALER_EXTFUNCCALLHOOK_H

#ifdef __linux

#include <string>
#include <util/tool/ElfParser.h>
#include <vector>
#include <util/tool/ProcInfoParser.h>
#include <util/tool/MemoryTool.h>
#include <exceptions/ScalerException.h>
#include <util/datastructure/Hashmap.hh>
#include <util/datastructure/Vector.h>
#include <type/ExtSymInfo.h>
#include <type/ELFImgInfo.h>
#include <type/ELFSecInfo.h>
#include <atomic>
#include "util/datastructure/MemoryHeapList.h"

namespace scaler {

    /**
    * Determine whether a symbol should be hooked
    */
    typedef bool SYMBOL_FILTER(std::string fileName, std::string funcName);

    const int ID_SAVER_BIN_SIZE=143;
    struct IdSaverBinWrapper {
        uint8_t idSaverBin[ID_SAVER_BIN_SIZE] = {
                /**
                 * Read TLS part
                 */
                //mov $0x1122334455667788,%r11 | move TLS offset to r11
                0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                //mov %fs:(%r11),%r11 | move TLS address to r11
                0x64, 0x4D, 0x8B, 0x1B,
                //cmpq $0,%r11 | Check TLS is initialized or not
                0x49, 0x83, 0xFB, 0x00,
                //je SKIP | If TLS initialized, do not invoke any hook
                0x74, 0x2D,

                /**
                 * Counting part
                 */
                //push %r10 | Save register r10
                0x41, 0x52,
                //mov    0x00000000(%r11),%r11 | mov the address to Context.ldArr[loadingId].internalArr to r11
                0x4D, 0x8B, 0x9B, 0x00, 0x00, 0x00, 0x00,
                //mov    0x00000000(%r11),%r10 | mov the value of current API's invocation counter to r10
                0x4D, 0x8B, 0x93, 0x00, 0x00, 0x00, 0x00,
                //add    $0x1,%r10 | Increase counter by 1
                0x49, 0x83, 0xC2, 0x01,
                //mov    %r10,0x11223344(%r11) | Store updated counter back
                0x4D, 0x89, 0x93, 0x00, 0x00, 0x00, 0x00,
                //mov    0x11223344(%r11),%r11 | move the value of current API's gap to r10
                0x4D, 0x8B, 0x9B, 0x00, 0x00, 0x00, 0x00,
                //and    %r11,%r10 | counter % gap
                0x4D, 0x21, 0xDA,
                //cmpq   $0x0,%r10 | Check the following if
                0x49, 0x83, 0xFA, 0x00,
                //pop    %r10 | Restore the value of r10
                0x41, 0x5A,
                //jz TIMING_JUMPER | If counter % gap == 0 skip, otherwise jump to TIMING part
                0x74, 0x1F,

                /**
                 * SKIP part
                 */
                //SKIP:
                //movq $0x1122334455667788,%r11 | Move current API's GOT address to Scaler's context
                0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                //jmpq (%r11) | Jump to the address of API's GOT entry
                0x41, 0xFF, 0x23,

                //pushq $0x11223344 | Save the ld.so id of this API to the stack. This value will be picked up by ld.so.
                0x68, 0x00, 0x00, 0x00, 0x00,
                //movq $0x1122334455667788,%r11 | move the address of PLT[0] to r11
                0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                //jmpq *%r11 | Jump to PLT[0]
                0x41, 0xFF, 0xE3,

                /**
                 * TIMING part
                 */

                //movl $0x44332211,0x28(%rsp) | Save low bits of gotEntrtAddress
                0xC7, 0x44, 0x24, 0xE8, 0x00, 0x00, 0x00, 0x00,
                //movl $0x44332211,0x10(%rsp) | Save hi bits of gotEntrtAddress
                0xC7, 0x44, 0x24, 0xEC, 0x00, 0x00, 0x00, 0x00,
                //movl $0x44332211,0x08(%rsp) | Save loadingId to stack
                0x48, 0xC7, 0x44, 0x24, 0xF0, 0x00, 0x00, 0x00, 0x00,
                //movl $0x44332211,0x0(%rsp) | Save funcId to stack
                0x48, 0xC7, 0x44, 0x24, 0xF8, 0x00, 0x00, 0x00, 0x00,
                //movq $0x1122334455667788,%r11 | Move the address of asmHookHandler to r11
                0x49, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                //jmpq *%r11 | Jump to asmHookHandler
                0x41, 0xFF, 0xE3
        };
        void* realFuncAddr;
    };

    class ExtFuncCallHook {
    public:
        ExtFuncCallHook(std::string folderName);

        ExtFuncCallHook(ExtFuncCallHook &) = delete;

        ExtFuncCallHook(ExtFuncCallHook &&) = delete;

        virtual bool install();

        virtual bool installDlOpen();

        virtual bool installDlSym(void *realFuncAddr,void*& retAddr);

        virtual bool uninstall();

        virtual ~ExtFuncCallHook();

        PmParser pmParser;

        FixedArray<Array<ELFImgInfo>> elfImgInfoMap;//Mapping fileID to ELFImgInfo.
        FixedArray<Array<ExtSymInfo>> allExtSymbol;//All external symbols in ELF image
        //Protect possible contention in thread context initialization. All context initialization/dynamic loading should acquire this lock.
        //But hook handler does not need to acquire this lock to save overhead.
        //Because there is no expansion possibility for elfImgInfoMap and allExtSymbol. Already initialized fields stays initialized and dynamic loading will not impact them.
        //Dynamically loaded fields will only be invoked by the loading thread or other thread after the loading completes.
        //Python has GIL, so there is no real simultaenous access. Besides, python modules are seperated with others by using different loading id.
        pthread_mutex_t dynamicLoadingLock;
        std::string folderName;
        std::atomic<ssize_t> curLoadingId;

        std::map<void*,uint8_t*> dlsymRealAddrGOTEntryMap; //Convert dynamically loaded symbol address to slots in memoryHeapList
        MemoryHeapList<IdSaverBinWrapper> memoryHeapList; //Holds idsaver of individual symbols

        /**
         * Private constructor
         */

        inline bool isSymbolAddrResolved(ssize_t loadingId, ExtSymInfo &symInfo) {
            //Check whether its value has 6 bytes offset as its plt entry start address
            ELFImgInfo &curImg = elfImgInfoMap[loadingId][symInfo.fileId];
            int64_t myPltStartAddr = (int64_t) curImg.pltStartAddr;
            int64_t curGotAddr = (int64_t) symInfo.gotEntryAddr;
            int64_t offset = curGotAddr - myPltStartAddr;
            return offset > 6 || offset < -6;
        }

        static ExtFuncCallHook *getInst(std::string folderName);

        static ExtFuncCallHook *getInst();

        static ExtFuncCallHook *instance;

    protected:
        virtual bool install(ssize_t loadingId);

        inline bool shouldHookThisSymbol(const char *funcName, Elf64_Word &bind, Elf64_Word &type, SymID curSymId,
                                         ssize_t &initialGap, void *&addressOverride);


        inline bool
        parseSecInfos(ELFParser &elfParser, ELFSecInfo &pltInfo, ELFSecInfo &pltSecInfo, ELFSecInfo &gotInfo,
                      uint8_t *baseAddr, uint8_t *startAddr, uint8_t *endAddr);

        bool
        parseSymbolInfo(ssize_t loadingId, ELFParser &parser, ssize_t fileId, uint8_t *baseAddr, ELFSecInfo &pltSection,
                        ELFSecInfo &pltSecureSection,
                        ELFSecInfo &gotSec, uint8_t *startAddr, uint8_t *endAddr);

        bool makeGOTWritable(ELFSecInfo &gotSec, bool writable);


        uint32_t parsePltStubId(uint8_t *dest);

        bool fillAddr2pltEntry(uint8_t *funcAddr, uint8_t *retPltEntry);

        bool fillAddrAndSymId2IdSaver(uint8_t **gotAddr, uint8_t *firstPltEntry, uint32_t symId,
                                      uint32_t pltStubId, uint32_t recArrayOffset, uint32_t loadingId,
                                      uint32_t countOffset, uint32_t gapOffset, uint8_t *idSaverEntry);


        void parseRequiredInfo(ssize_t loadingId);

        /**
         * Actual entry
         * @return
         */
        bool replacePltEntry(ssize_t loadingId);

        void createRecordingFolder() const;

    };

}


extern "C" {


__pid_t fork(void);

}


#endif

#endif
