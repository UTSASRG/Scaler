#ifndef SCALER_EXTFUNCCALLHOOK_BRKPOINT_H
#define SCALER_EXTFUNCCALLHOOK_BRKPOINT_H


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
#include <util/tool/AddrFileIdMapping.h>
#include <util/hook/ExtFuncCallHook.h>
#include <type/Breakpoint.h>


namespace scaler {

    class ExtFuncCallHookBrkpoint : public ExtFuncCallHook {
    public:
        ExtFuncCallHookBrkpoint(std::string folderName);

        ExtFuncCallHookBrkpoint(ExtFuncCallHookBrkpoint &) = delete;

        ExtFuncCallHookBrkpoint(ExtFuncCallHookBrkpoint &&) = delete;

        bool install() override;

        bool uninstall() override;

        ~ExtFuncCallHookBrkpoint() override;

        void registerSigIntHandler();

        static ExtFuncCallHookBrkpoint *getInst(std::string folderName);

        static ExtFuncCallHookBrkpoint *instance;

        void insertBrkpoints();

        void recordBrkpointInfo(const FuncID &funcID, void *addr, bool isPLT);

        HashMap<void *, Breakpoint> brkPointInfo;

        std::set<void *> brkpointPltAddr;

        void insertBrkpointAt(Breakpoint &bp);
    };

}


#endif
