
#include <util/hook/HookHandlersBrkpoint.h>
#include <util/hook/ExtFuncCallHookBrkpoint.h>
#include <util/tool/VMEmulator.h>


void brkpointEmitted(int signum, siginfo_t *siginfo, void *context) {
//    printf("brkpointEmitted\n");
    //DBG_LOG("brkpointEmitted");
    auto thiz = scaler::ExtFuncCallHookBrkpoint::getInst("");

    greg_t regRip = ((ucontext_t *) context)->uc_mcontext.gregs[REG_RIP];

    void *pltPtr = (void *) ((uint64_t) regRip - 1); //The plt address where we applied breakpoint to

    scaler::Breakpoint *bp = thiz->brkPointInfo.getPtr(pltPtr);
    skipBrkPoint(*bp, static_cast<ucontext_t *>(context));
}


void skipBrkPoint(scaler::Breakpoint &bp, ucontext_t *context) {
    //Otherwise we should execute
    //todo: Assert instruction is valid as it is guaranteed in brkpointEmitted
    auto thiz = scaler::ExtFuncCallHookBrkpoint::getInst("");

    if (bp.fileID == -1) {
        //fileID unresolved, recolve now
        bp.fileID = thiz->pmParser.findExecNameByAddr(bp.addr);
    }

    scaler::VMEmulator &emulator = scaler::VMEmulator::getInstance();

    xed_uint64_t relativeAddr = (xed_uint64_t) bp.addr;
    bool isapp = thiz->pmParser.findExecNameByAddr(bp.addr) == 0;
//        if (!isapp) {
//            //todo: Change everything to intptr
//            relativeAddr = (xed_uint64_t) ((intptr_t) bp.addr - (intptr_t) pmParser.fileBaseAddrMap.at(bp.fileID).first);
//        }
    DBG_LOGS("relativeAddr=%lu %p isApp=%s", relativeAddr, bp.addr, isapp ? "true" : "false");
    emulator.parseOp(bp.xedDecodedInst, relativeAddr, bp.operands, OPERAND_NUMBER, context);

    //Check the type of original code. If it is jmp, then we shouldn't use assembly to execute but should modify rip directly
    if (bp.xiclass >= XED_ICLASS_JB && bp.xiclass <= XED_ICLASS_JZ) {
        bool shouldJmp = false;
        long long flags = context->uc_mcontext.gregs[REG_EFL];
        long cf_mask = 0x0001;
        long pf_mask = 0x0004;
        //long af_mask = 0x0010;
        long zf_mask = 0x0040;
        long sf_mask = 0x0080;
        long of_mask = 0x0800;

        switch (bp.xiclass) {
            case XED_ICLASS_JB: {
                // (CF=1)
                shouldJmp = cf_mask & flags;
                break;
            }
            case XED_ICLASS_JBE: {
                // CF=1 or ZF=1
                shouldJmp = (cf_mask & flags) || (zf_mask & flags);
                break;
            }
            case XED_ICLASS_JCXZ: {
                // CX register is 0
                size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
                size_t cx = rcx_value & 0xFFFF;
                shouldJmp = (cx == 0);
                break;
            }
            case XED_ICLASS_JECXZ: {
                // ECX register is 0
                size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
                size_t ecx = rcx_value & 0xFFFFFFFF;
                shouldJmp = (ecx == 0);
                break;
            }
            case XED_ICLASS_JL: {
                // SF≠ OF
                shouldJmp = (sf_mask & flags) ^ (of_mask & flags);
                break;
            }
            case XED_ICLASS_JLE: {
                // (ZF=1 or SF ≠ OF)
                shouldJmp = (zf_mask & flags) || ((sf_mask & flags) ^ (of_mask & flags));
                break;
            }
            case XED_ICLASS_JNB: {
                // (CF=0)
                shouldJmp = !(cf_mask & flags);
                break;
            }
            case XED_ICLASS_JNBE: {
                // (CF=0 and ZF=0)
                shouldJmp = !(cf_mask & flags) && !(zf_mask & flags);
                break;
            }
            case XED_ICLASS_JNL: {
                // (SF=OF)
                shouldJmp = !((sf_mask & flags) ^ (of_mask & flags));
                break;
            }
            case XED_ICLASS_JNLE: {
                // (ZF=0 and SF=OF)
                shouldJmp = !(zf_mask & flags) && !((sf_mask & flags) ^ (of_mask & flags));
                break;
            }
            case XED_ICLASS_JNO: {
                // (OF=0)
                shouldJmp = !(of_mask & flags);
                break;
            }
            case XED_ICLASS_JNP: {
                // (PF=0)
                shouldJmp = !(pf_mask & flags);
                break;
            }
            case XED_ICLASS_JNS: {
                // (SF=0)
                shouldJmp = !(sf_mask & flags);
                break;
            }
            case XED_ICLASS_JNZ: {
                // (ZF=0)
                shouldJmp = !(zf_mask & flags);
                break;
            }
            case XED_ICLASS_JO: {
                // (OF=1)
                shouldJmp = of_mask & flags;
                break;
            }
            case XED_ICLASS_JP: {
                // (PF=1)
                shouldJmp = pf_mask & flags;
                break;
            }
            case XED_ICLASS_JRCXZ: {
                // RCX register is 0
                size_t rcx_value = GET_REG_VALUE(context, REG_RCX);
                shouldJmp = (rcx_value == 0);
                break;
            }
            case XED_ICLASS_JS: {
                // (SF=1)
                shouldJmp = sf_mask & flags;
                break;
            }
            case XED_ICLASS_JZ: {
                // (ZF=1)
                shouldJmp = zf_mask & flags;
                break;
            }
            case XED_ICLASS_JMP:
            case XED_ICLASS_JMP_FAR: {
                shouldJmp = true;
                break;
            }
            default: { fatalError("Instruction is jmp but scaler doesn't recognize it");
            }
        }

        if (shouldJmp) {
            //near jump
            size_t jmpTarget = scaler::VMEmulator::getInstance().getOp(bp.operands[0]);
            //far jump

            void *nextInstrAddr = (void *) jmpTarget;

            if (xed3_operand_get_nominal_opcode(&bp.xedDecodedInst) != 0xFF && !isapp) {
                nextInstrAddr = (void *) ((intptr_t) thiz->pmParser.pmEntryArray[0].addrStart + jmpTarget);
            }

            //Change pc and jmp to this instruction directly
            context->uc_mcontext.gregs[REG_RIP] = (intptr_t) nextInstrAddr;
        } else {
            //Change pc and jmp to this instruction directly
            context->uc_mcontext.gregs[REG_RIP] = (intptr_t) bp.addr + bp.instLen;
        }

    } else {
        DBG_LOG("Return complete2");
        assert(false);
//            //Normal instruciton. Execute directly using assembly code;
//            MemoryTool_Linux::getInst()->adjustMemPerm(brkpointCurContext.instrExecArea,
//                                                       (uint8_t *) brkpointCurContext.instrExecArea +
//                                                       INSTR_EXEC_AREA_LEN,
//                                                       PROT_READ | PROT_WRITE | PROT_EXEC);
//            //Copy the first instruction
//            memcpy(brkpointCurContext.instrExecArea, bp.oriCode, bp.instLen);
//            assert(sizeof(bp.oriCode) < sizeof(brkpointCurContext.instrExecArea) - 1);
//            brkpointCurContext.instrExecArea[bp.instLen] = 0xCB;
//
//            //Fill memory address
//            asm volatile (
//            "movq %[aInstrExecArea],%%r11\n\t"
//            "callq *%%r11\n\t"
//            :  //output
//            : [aInstrExecArea] "rm"(brkpointCurContext.instrExecArea) //input
//            : //clobbers
//            );
    }
}