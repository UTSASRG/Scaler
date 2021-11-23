#include <util/tool/MemToolPtrace.h>
#include <sys/ptrace.h>
#include <util/tool/Logging.h>
#include <wait.h>
#include <sys/user.h>

//Initialize instance
scaler::MemoryToolPtrace *scaler::MemoryToolPtrace::instance = nullptr;

scaler::MemoryToolPtrace *scaler::MemoryToolPtrace::getInst(PmParser_Linux &pmParser) {
    //MemoryTool_Linux::instance memory leak
    if (MemoryToolPtrace::instance == nullptr) {
        MemoryToolPtrace::instance = new MemoryToolPtrace(pmParser);
    }
    return MemoryToolPtrace::instance;
}

scaler::MemoryToolPtrace::MemoryToolPtrace(PmParser_Linux &pmParser) : pmParser(pmParser) {

}

scaler::MemoryToolPtrace::~MemoryToolPtrace() {

}

void *scaler::MemoryToolPtrace::binCodeSearch(void *target, ssize_t targetSize, void *keyword, ssize_t keywordSize) {

    void *cpyDataAddr = pmParser.readProcMem(target, targetSize);
    if (cpyDataAddr == nullptr)
        return nullptr;

    void *searchRlt = MemoryTool::binCodeSearch(cpyDataAddr, targetSize, keyword, keywordSize);
    free(cpyDataAddr);

    if (searchRlt == nullptr)
        return nullptr;

    void *realAddr = (void *) ((unsigned long long) (target) + (unsigned long long) searchRlt -
                               (unsigned long long) cpyDataAddr);
    return realAddr;
}
