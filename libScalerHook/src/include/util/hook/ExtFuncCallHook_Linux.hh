#ifndef SCALER_EXTFUNCCALLHOOK_LINUX_HH
#define SCALER_EXTFUNCCALLHOOK_LINUX_HH

#include <util/hook/hook.hh>

#ifdef __linux

#include <string>
#include <util/tool/ElfParser.h>
#include <vector>
#include <util/tool/ProcInfoParser.h>
#include <util/tool/MemTool.h>
#include <util/tool/MemoryTool_Linux.h>
#include <exceptions/ScalerException.h>
#include <util/datastructure/Hashmap.hh>
#include <util/datastructure/Vector.h>

namespace scaler {

    class ExtFuncCallHook_Linux : public Hook {
    public:

        virtual void install(SYMBOL_FILTER filterCallB) override = 0;


        virtual void uninstall() override = 0;

        ExtFuncCallHook_Linux(ExtFuncCallHook_Linux &) = delete;

        ExtFuncCallHook_Linux(ExtFuncCallHook_Linux &&) = delete;

        virtual ~ExtFuncCallHook_Linux() override = 0;

        virtual void parseFuncInfo(FileID callerFileID, SymID symbolIDInCaller, void *&funcAddr, FileID &libraryFileID);

        virtual void saveAllSymbolId();

        /**
        * Symbol information
        */
        class ExtSymInfo {
        public:
            std::string symbolName = "";     //The name of a symbol in symbol table
            void **gotEntry = nullptr;            //Pointer to a symbol's GOT entry. Use *gotEntry to get this symbol's real address.
            void *pltEntry = nullptr;            //Pointer to a symbol's PLT entry.
            void *pltSecEntry = nullptr;            //Pointer to a symbol's .plt.sec entry.
            void *addr = nullptr;                 //The address of a symbol. After a symbol is resolved, it's equal to *gotEntry;
            FileID fileId = -1;             //Store fileID for this symbol
            SymID extSymbolId = -1;             //The id with respect to where this symbol is called. Store this symbol's ID (it's also called symbolID) //todo: change this to symbolID for consistency
            FileID libraryFileID = -1;       //Store the libary file id that contains this
            void *pseudoPltEntry = nullptr;                   //A pointer to pseudoPltEntry
            int type = -1;
            int bind = -1;

            bool operator==(const ExtSymInfo &rho) const {
                if (&rho != this) {
                    return fileId == rho.fileId && extSymbolId == rho.extSymbolId;
                } else {
                    return true;
                }
            }
        };

        /**
         * ELF image (ELF file in memory) information.
         */
        class ELFImgInfo {
        public:

            struct PthreadFuncId {
                int PTHREAD_CREATE = -1;
                int PTHREAD_JOIN = -1;
                int PTHREAD_TRYJOIN_NP = -1;
                int PTHREAD_TIMEDJOIN_NP = -1;
                int PTHREAD_CLOCKJOIN_NP = -1;
                int PTHREAD_MUTEX_LOCK = -1;
                int PTHREAD_MUTEX_TIMEDLOCK = -1;
                int PTHREAD_MUTEX_CLOCKLOCK = -1;
                int PTHREAD_MUTEX_UNLOCK = -1;
                int PTHREAD_RWLOCK_RDLOCK = -1;
                int PTHREAD_RWLOCK_TRYRDLOCK = -1;
                int PTHREAD_RWLOCK_TIMEDRDLOCK = -1;
                int PTHREAD_RWLOCK_CLOCKRDLOCK = -1;
                int PTHREAD_RWLOCK_WRLOCK = -1;
                int PTHREAD_RWLOCK_TRYWRLOCK = -1;
                int PTHREAD_RWLOCK_TIMEDWRLOCK = -1;
                int PTHREAD_RWLOCK_CLOCKWRLOCK = -1;
                int PTHREAD_RWLOCK_UNLOCK = -1;
                int PTHREAD_COND_SIGNAL = -1;
                int PTHREAD_COND_BROADCAST = -1;
                int PTHREAD_COND_WAIT = -1;
                int PTHREAD_COND_TIMEDWAIT = -1;
                int PTHREAD_COND_CLOCKWAIT = -1;
                int PTHREAD_SPIN_LOCK = -1;
                int PTHREAD_SPIN_TRYLOCK = -1;
                int PTHREAD_SPIN_UNLOCK = -1;
                int PTHREAD_BARRIER_WAIT = -1;

                bool isFuncPthread(FuncID funcID);

                std::vector<int> getAllIds();

            };

            struct SemaphoreFuncId {
                int SEM_WAIT = -1;
                int SEM_TIMEDWAIT = -1;
                int SEM_CLOCKWAIT = -1;
                int SEM_TRYWAIT = -1;
                int SEM_POST = -1;

                bool isFuncSemaphore(FuncID funcID);

                std::vector<int> getAllIds();
            };


            std::string filePath = "";                           //The absolute path of an ELF image (It's consistent with /proc/{pid}/maps)
            void *pltStartAddr = nullptr;                   //The starting address of the PLT table
            void *pltEndAddr = nullptr;                     //The ending address of the PLT table
            void *pltSecStartAddr = nullptr;                //The starting address of the PLT.SEC table
            void *pltSecEndAddr = nullptr;                  //The ending address of the PLT.SEC table
            ElfW(Dyn) *_DYNAMICAddr = nullptr;              //The staring address of _DYNAMIC

            Vector<bool> realAddrResolved;             //Whether function with id i has been resolved.


//            uint8_t *pseudoPlt = nullptr;                   //A pointer to pseudoPlt table

            HashMap<SymID, ExtSymInfo> hookedExtSymbol;   //External symbols that has already been hooked
            //todo:memory leak
            HashMap<SymID, void *> oriPltCode;   //External symbols that has already been hooked
            HashMap<SymID, void *> oriPltSecCode;   //External symbols that has already been hooked


            std::map<SymID, ExtSymInfo> allExtSymbol;      //All external symbols in ELF image
            std::map<std::string, FuncID> funcIdMap;        //Mapping function name to it's id
            std::map<FuncID, std::string> idFuncMap;        //Mapping function id to it's name
            //todo: Check const for all variables
            ElfW(Rela) *relaPlt = nullptr;                            //The first .plt.rela entry in ELF iamge
            ElfW(Xword) relaPltCnt = 0;                         //The number of entries in relaPlt
            const ElfW(Sym) *dynSymTable = nullptr;                   //The first .dynamic entry in ELF image
            const char *dynStrTable = nullptr;                        //The starting position of dynamic symbol name
            ssize_t dynStrSize = 0;                              //The size of dynamic string table

            uint8_t *baseAddrStart = nullptr;                              //The loading address of current elf image
            uint8_t *baseAddrEnd = nullptr;                              //The loading address of current elf image

            PthreadFuncId pthreadExtSymbolId;

            SemaphoreFuncId semaphoreExtSymbolId;


            ELFImgInfo();


        };


        PmParser_Linux &pmParser;
        MemoryTool_Linux &memTool;

        HashMap<FileID, ELFImgInfo> elfImgInfoMap;         //Mapping fileID to ELFImgInfo


        /**
         * Private constructor
         */
        ExtFuncCallHook_Linux(PmParser_Linux &parser, MemoryTool_Linux &memTool);

        /**
        * Locate the address of required sections in memory
        */
        virtual void locateRequiredSecAndSeg();

        /**
        * Find elf section in memory and return start and end address
        */
        virtual void
        findELFSecInMemory(ELFParser_Linux &elfParser, std::string secName, void *&startAddr, void *endAddr,
                           void *boundStartAddr, void *boundEndAddr);

        virtual Elf64_Dyn *findDynamicSegment(ELFParser_Linux &elfParser);

        template<typename SEGTYPE>
        SEGTYPE findElemPtrInDynamicSeg(ELFParser_Linux &elfParser,
                                        ELFImgInfo &curELFImgInfo,
                                        FileID curFileID,
                                        ElfW(Sxword) elemType) {

            const ElfW(Dyn) *dynEntryPtr = elfParser.findDynEntryByTag(curELFImgInfo._DYNAMICAddr, elemType);
            if (dynEntryPtr == nullptr) {
                throwScalerException(-1, "Cannot find a dyn entry");
            }
            uint8_t *curBaseAddr = pmParser.autoAddBaseAddr(curELFImgInfo.baseAddrStart, curFileID,
                                                            dynEntryPtr->d_un.d_ptr);
            return (SEGTYPE) (curBaseAddr + dynEntryPtr->d_un.d_ptr);
        }

        template<typename SEGTYPE>
        SEGTYPE findElemValInDynamicSeg(ELFParser_Linux &elfParser,
                                        ELFImgInfo &curELFImgInfo,
                                        FileID curFileID,
                                        ElfW(Sxword) elemType) {

            const ElfW(Dyn) *dynEntryPtr = elfParser.findDynEntryByTag(curELFImgInfo._DYNAMICAddr, elemType);
            if (dynEntryPtr == nullptr) {
                throwScalerException(0, "Cannot find a dyn entry");
            }
            return (SEGTYPE) (dynEntryPtr->d_un.d_val);
        }


        virtual void parseRelaSymbol(ELFImgInfo &curELFImgInfo, FileID curFileID);
    };

}


#endif

#endif
