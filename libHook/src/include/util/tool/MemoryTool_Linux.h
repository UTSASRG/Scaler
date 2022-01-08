//
// Created by st on 7/6/21.
//

#ifndef SCALER_MEMORYTOOL_LINUX_H
#define SCALER_MEMORYTOOL_LINUX_H


#include "MemTool.h"

namespace scaler {

    class MemoryTool_Linux : public MemoryTool {
    public:
        /**
         * Singeleton
         */
        static MemoryTool_Linux *getInst();


        bool adjustMemPerm(void *startPtr, void *endPtr, int prem);

        ~MemoryTool_Linux() override;


    protected:
        //Singeleton
        MemoryTool_Linux();

        static MemoryTool_Linux *instance;

    };
}

#endif //SCALER_MEMORYTOOL_LINUX_H
