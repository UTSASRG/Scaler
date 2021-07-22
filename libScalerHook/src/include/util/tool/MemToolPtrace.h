//
// Created by st on 7/6/21.
//

#ifndef SCALER_MEMTOOLPTRACE_H
#define SCALER_MEMTOOLPTRACE_H

#include "MemTool.h"
#include "MemoryTool_Linux.h"

namespace scaler {
    class MemoryToolPtrace : public MemoryTool_Linux {
    public:
        /**
         * Singeleton
         */
        static MemoryToolPtrace *getInst(PmParser_Linux& pmParser);

        ~MemoryToolPtrace() override;


        void *binCodeSearch(void *target, size_t targetSize, void *keyword, size_t keywordSize) override;


    protected:
        //Singeleton
        explicit MemoryToolPtrace(PmParser_Linux& pmParser);

        static MemoryToolPtrace *instance;
        PmParser_Linux& pmParser;
    };
}

#endif //SCALER_MEMTOOLPTRACE_H
