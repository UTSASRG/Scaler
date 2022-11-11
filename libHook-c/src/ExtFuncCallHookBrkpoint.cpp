#include <util/hook/ExtFuncCallHookBrkpoint.h>
#include <csignal>
#include <util/hook/HookHandlersBrkpoint.h>
#include <util/tool/VMEmulator.h>
#include <util/tool/MemoryTool.h>

namespace scaler {

    ExtFuncCallHookBrkpoint *ExtFuncCallHookBrkpoint::instance = nullptr;


    ExtFuncCallHookBrkpoint *ExtFuncCallHookBrkpoint::getInst(std::string folderName) {
        if (!instance) {
            instance = new ExtFuncCallHookBrkpoint(folderName);
            ExtFuncCallHook::instance = instance;
            if (!instance) { fatalError("Cannot allocate memory for ExtFuncCallHookAsm");
                return nullptr;
            }
        }
        return instance;
    }


    bool ExtFuncCallHookBrkpoint::install() {
        createRecordingFolder();

        std::stringstream ss;
        ss << scaler::ExtFuncCallHookBrkpoint::instance->folderName << "/symbolInfo.txt";
        FILE *symInfoFile = fopen(ss.str().c_str(), "a");
        if (!symInfoFile) { fatalErrorS("Cannot open %s because:%s", ss.str().c_str(), strerror(errno))
        }
        fprintf(symInfoFile, "%s,%s,%s\n", "funcName", "fileId", "symIdInFile");
        fclose(symInfoFile);




        //Parse filenames
        pmParser.parsePMMap();
        //Get pltBaseAddr

        parseRequiredInfo();

        registerSigIntHandler();

        insertBrkpoints();


        DBG_LOG("Finished installation");

        return true;
    }

    bool ExtFuncCallHookBrkpoint::uninstall() {
        return true;
    }

    ExtFuncCallHookBrkpoint::~ExtFuncCallHookBrkpoint() {

    }

    void ExtFuncCallHookBrkpoint::registerSigIntHandler() {

        struct sigaction sig_trap;
        // block the signal, if it is already inside signal handler
        sigemptyset(&sig_trap.sa_mask);
        sigaddset(&sig_trap.sa_mask, SIGTRAP);
        //set sa_sigaction handler
        sig_trap.sa_flags = SA_SIGINFO;
        sig_trap.sa_sigaction = brkpointEmitted;
        // register signal handler
        if (sigaction(SIGTRAP, &sig_trap, NULL) == -1) { fatalError("Cannot succesfully set signal breakpoint");
        }

    }

    void ExtFuncCallHookBrkpoint::insertBrkpoints() {

        /**
         * Replace plt entry or replace .plt (Or directly replace .plt.sec)
         */
        for (int curSymId = 0; curSymId < allExtSymbol.getSize(); ++curSymId) {
            ExtSymInfo &curSym = allExtSymbol[curSymId];
            ELFImgInfo &curImgInfo = elfImgInfoMap[curSym.fileId];


            if (curSym.pltEntryAddr) {
                recordBrkpointInfo(curSymId, curSym.pltEntryAddr, true);
                //Install here
            } else { fatalError("PltEntry is None!")
            }

        }


        for (int curSymId = 0; curSymId < allExtSymbol.getSize(); ++curSymId) {
            ExtSymInfo &curSym = allExtSymbol[curSymId];
            ELFImgInfo &curImgInfo = elfImgInfoMap[curSym.fileId];


            //Actual install, this is to prevent extra function call inside libxed code.
            Breakpoint &bp = brkPointInfo.get(curSym.pltEntryAddr);
            insertBrkpointAt(bp);
        }

    }

    pthread_mutex_t lockBrkpointOp = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

    void ExtFuncCallHookBrkpoint::insertBrkpointAt(Breakpoint &bp) {
        pthread_mutex_lock(&lockBrkpointOp);

        adjustMemPerm(bp.addr,
                      (uint8_t *) bp.addr + bp.instLen,
                      PROT_READ | PROT_WRITE | PROT_EXEC);

        bp.addr[0] = 0xCC; //Insert 0xCC to the first byte

        adjustMemPerm(bp.addr,
                      (uint8_t *) bp.addr + bp.instLen,
                      PROT_READ | PROT_EXEC);

        pthread_mutex_unlock(&lockBrkpointOp);
    }

    void ExtFuncCallHookBrkpoint::recordBrkpointInfo(const FuncID &funcID, void *addr, bool isPLT) {
        //Get the plt data of curSymbol
        //todo: .plt size is hard coded

        Breakpoint bp;
        bp.addr = static_cast<uint8_t *>(addr);
        bp.funcID = funcID;

        //Parse instruction
        //todo: what if fail
        VMEmulator::getInstance().getInstrInfo(bp);

        brkPointInfo.put(addr, bp);


        if (isPLT)
            brkpointPltAddr.insert(addr);
    }

    ExtFuncCallHookBrkpoint::ExtFuncCallHookBrkpoint(std::string folderName) : ExtFuncCallHook(folderName) {

    }


}