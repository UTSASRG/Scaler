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

int attach(int start, int pid) {
    long ret;

    if (start == 1) {
        ret = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    } else {
        ret = ptrace(PTRACE_DETACH, pid, NULL, NULL);
    }

    if (ret < 0) {
        ERR_LOG("Could not attach to pid\n");
        exit(-1);
    }

    waitpid(pid, NULL, WUNTRACED);


    return (ret);
}

void *scaler::MemoryToolPtrace::binCodeSearch(void *target, size_t targetSize, void *keyword, size_t keywordSize) {

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
