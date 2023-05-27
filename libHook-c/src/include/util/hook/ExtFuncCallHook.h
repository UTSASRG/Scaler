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

namespace scaler {

    /**
    * Determine whether a symbol should be hooked
    */
    typedef bool SYMBOL_FILTER(std::string fileName, std::string funcName);

    class ExtFuncCallHook {
    public:
        ExtFuncCallHook(std::string folderName);

        ExtFuncCallHook(ExtFuncCallHook &) = delete;

        ExtFuncCallHook(ExtFuncCallHook &&) = delete;

        virtual bool install();

        virtual bool installAutoLoadingId();


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
                                      uint32_t pltStubId, uint32_t ldArrayOffset, uint32_t loadingId,
                                      uint32_t countOffset, uint32_t gapOffset, uint8_t *idSaverEntry);


        void parseRequiredInfo(ssize_t loadingId);

        /**
         * Actual entry
         * @return
         */
        bool replacePltEntry(ssize_t loadingId);

        void createRecordingFolder() const;

        void populateRecordingArray(ssize_t id);
    };

}

extern "C" {


__pid_t fork(void);

}


#endif

#endif
