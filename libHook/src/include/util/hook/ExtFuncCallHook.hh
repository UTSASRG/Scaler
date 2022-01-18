#ifndef SCALER_EXTFUNCCALLHOOK_HH
#define SCALER_EXTFUNCCALLHOOK_HH

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

    class ExtFuncCallHook : public Hook {
    public:

        virtual bool install(SYMBOL_FILTER filterCallB) override = 0;


        virtual bool uninstall() override = 0;

        ExtFuncCallHook(ExtFuncCallHook &) = delete;

        ExtFuncCallHook(ExtFuncCallHook &&) = delete;

        virtual ~ExtFuncCallHook() override = 0;

        virtual bool parseFuncInfo(FileID callerFileID, SymID symbolIDInCaller, void *&funcAddr, FileID &libraryFileID);

        virtual bool saveAllSymbolId();

        /**
        * Symbol information
        */
        class ExtSymInfo {
        public:
            std::string symbolName = "";     //The name of a symbol in symbol table
            void **gotEntry = nullptr;            //Pointer to a symbol's GOT entry. Use *gotEntry to get this symbol's real address.
            void *pltEntry = nullptr;            //Pointer to a symbol's PLT entry.
            void *pltSecEntry = nullptr;            //Pointer to a symbol's .plt.sec entry.
            void *addr = nullptr;                 //The address of a symbol. After a symbol is resolved, it's equal to *gotEntry; If the symbol is not resolve, it equals nullptr
            FileID fileId = -1;             //Store fileID for this symbol
            SymID symIdInFile = -1;             //The id with respect to where this symbol is called. Store this symbol's ID (it's also called symbolID) //todo: change this to symbolID for consistency
            SymID scalerSymbolId = -1; //todo: change the definition of symbol id so we only need to save one id. (Put all symbols in one list, one simple id corresponds to one symbol in a file. Same simple in different file is considered as different)
            FileID libraryFileID = -1;       //Store the libary file id that contains this
            void *pseudoPltEntry = nullptr;                   //A pointer to pseudoPltEntry
            int type = -1;
            int bind = -1;
            int hookedId = -1; //Indicating whether this symbol has been hooked or not. If it is hooked, hookedId is the index in
            void *oriPltSecCode = nullptr;
            void *oriPltCode = nullptr;

            bool operator==(const ExtSymInfo &rho) const {
                if (&rho != this) {
                    return fileId == rho.fileId && symIdInFile == rho.symIdInFile;
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
                SymID PTHREAD_CREATE = -1;
                SymID PTHREAD_JOIN = -1;
                SymID PTHREAD_TRYJOIN_NP = -1;
                SymID PTHREAD_TIMEDJOIN_NP = -1;
                SymID PTHREAD_CLOCKJOIN_NP = -1;
                SymID PTHREAD_MUTEX_LOCK = -1;
                SymID PTHREAD_MUTEX_TIMEDLOCK = -1;
                SymID PTHREAD_MUTEX_CLOCKLOCK = -1;
                SymID PTHREAD_MUTEX_UNLOCK = -1;
                SymID PTHREAD_RWLOCK_RDLOCK = -1;
                SymID PTHREAD_RWLOCK_TRYRDLOCK = -1;
                SymID PTHREAD_RWLOCK_TIMEDRDLOCK = -1;
                SymID PTHREAD_RWLOCK_CLOCKRDLOCK = -1;
                SymID PTHREAD_RWLOCK_WRLOCK = -1;
                SymID PTHREAD_RWLOCK_TRYWRLOCK = -1;
                SymID PTHREAD_RWLOCK_TIMEDWRLOCK = -1;
                SymID PTHREAD_RWLOCK_CLOCKWRLOCK = -1;
                SymID PTHREAD_RWLOCK_UNLOCK = -1;
                SymID PTHREAD_COND_SIGNAL = -1;
                SymID PTHREAD_COND_BROADCAST = -1;
                SymID PTHREAD_COND_WAIT = -1;
                SymID PTHREAD_COND_TIMEDWAIT = -1;
                SymID PTHREAD_COND_CLOCKWAIT = -1;
                SymID PTHREAD_SPIN_LOCK = -1;
                SymID PTHREAD_SPIN_TRYLOCK = -1;
                SymID PTHREAD_SPIN_UNLOCK = -1;
                SymID PTHREAD_BARRIER_WAIT = -1;

                bool isFuncPthread(FuncID funcID);

                /**
                 * This function returns the scaler id of all functions that belongs to pthread library
                 * @return
                 */
                bool getAllIds(std::vector<SymID> &retSymId);

            };

            struct SemaphoreFuncId {
                SymID SEM_WAIT = -1;
                SymID SEM_TIMEDWAIT = -1;
                SymID SEM_CLOCKWAIT = -1;
                SymID SEM_TRYWAIT = -1;
                SymID SEM_POST = -1;

                bool isFuncSemaphore(FuncID funcID);

                std::vector<SymID> getAllIds();
            };


            std::string filePath = "";                           //The absolute path of an ELF image (It's consistent with /proc/{pid}/maps)
            bool elfImgValid = true;
            void *pltStartAddr = nullptr;                   //The starting address of the PLT table
            void *pltEndAddr = nullptr;                     //The ending address of the PLT table
            void *pltSecStartAddr = nullptr;                //The starting address of the PLT.SEC table
            void *pltSecEndAddr = nullptr;                  //The ending address of the PLT.SEC table
            ElfW(Dyn) *_DYNAMICAddr = nullptr;              //The staring address of _DYNAMIC
            std::vector<SymID> scalerIdMap;
            // uint8_t *pseudoPlt = nullptr;                   //A pointer to pseudoPlt table

            //todo:memory leak

            //std::map<std::string, FuncID> funcIdMap;        //Mapping function name to it's id
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

        Vector<ELFImgInfo> elfImgInfoMap;//Mapping fileID to ELFImgInfo
        Vector<SymID> hookedExtSymbol;//External symbol ids that has already been hooked
        Vector<ExtSymInfo> allExtSymbol;//All external symbols in ELF image

        /**
         * Private constructor
         */
        ExtFuncCallHook(PmParser_Linux &parser, MemoryTool_Linux &memTool);

        /**
        * Locate the address of required sections in memory
        */
        virtual bool locateRequiredSecAndSeg();

        /**
        * Find elf section in memory and return start and end address
        */
        virtual bool
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
                ERR_LOG("Cannot find a dyn entry");
                return nullptr;
            }
            //DBG_LOGS("d_tag=%16lx d_value=%16lx", dynEntryPtr->d_tag, dynEntryPtr->d_un.d_val);
            uint8_t *curBaseAddr = pmParser.autoAddBaseAddr(curELFImgInfo.baseAddrStart, curFileID,
                                                            dynEntryPtr->d_un.d_ptr);
//            DBG_LOGS("curBaseAddr(%p) + dynEntryPtr->d_un.d_ptr(%p) =%16lx", (void *) curBaseAddr,
//                     (void *) dynEntryPtr->d_un.d_ptr,
//                     (long unsigned int) curBaseAddr + dynEntryPtr->d_un.d_ptr);
            return (SEGTYPE) (curBaseAddr + dynEntryPtr->d_un.d_ptr);
        }

        template<typename SEGTYPE>
        SEGTYPE findElemValInDynamicSeg(ELFParser_Linux &elfParser,
                                        ELFImgInfo &curELFImgInfo,
                                        FileID curFileID,
                                        ElfW(Sxword) elemType) {

            const ElfW(Dyn) *dynEntryPtr = elfParser.findDynEntryByTag(curELFImgInfo._DYNAMICAddr, elemType);
            if (dynEntryPtr == nullptr) {
                ERR_LOG("Cannot find a dyn entry");
                return 0;
            }
            return (SEGTYPE) (dynEntryPtr->d_un.d_val);
        }


        virtual bool parseRelaSymbol(ELFImgInfo &curELFImgInfo, FileID curFileID);

        inline bool isSymbolAddrResolved(ExtSymInfo &symInfo) {
            //Check whether its value has 6 bytes offset as its plt entry start address
            int64_t myPltStartAddr = (int64_t) symInfo.pltEntry;
            int64_t curGotAddr = (int64_t) *symInfo.gotEntry;
            assert(symInfo.pltEntry != nullptr);
            int64_t offset = curGotAddr - myPltStartAddr;
            return offset > 6 || offset < -6;
        }
    };

}


#endif

#endif
