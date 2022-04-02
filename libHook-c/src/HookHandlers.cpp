
#include <util/hook/HookHandlers.h>
#include <type/ExtSymInfo.h>
#include <util/hook/ExtFuncCallHook.h>
#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>

#define PUSHZMM(ArgumentName) \
"subq $64,%rsp\n\t" \
"vmovdqu64  %zmm"#ArgumentName" ,(%rsp)\n\t"

#define POPZMM(ArgumentName) \
"vmovdqu64  (%rsp),%zmm"#ArgumentName"\n\t"\
"addq $64,%rsp\n\t"

/**
* Restore Registers
*/

//    POPZMM(7) \
//    POPZMM(6)\
//    POPZMM(5)\
//    POPZMM(4)\
//    POPZMM(3)\
//    POPZMM(2)\
//    POPZMM(1)\
//    POPZMM(0)
#define ASM_RESTORE_ENV_PREHOOK \
    "popq %r10\n\t"\
    "popq %r9\n\t"\
    "popq %r8\n\t"\
    "popq %rdi\n\t"\
    "popq %rsi\n\t"\
    "popq %rdx\n\t"\
    "popq %rcx\n\t"\
    "popq %rax\n\t"


/**
 * Source code version for #define IMPL_ASMHANDLER
 * We can't add comments to a macro
 *
 * Input stack
 *
 * oldrsp-152  currsp+0
 *                      8 bytes caller address
 * oldrsp-145  currsp+7
 *
 *
 * oldrsp-144  currsp+8
 *                      8 bytes funcID
 * oldrsp-137  currsp+15
 *
 *
 * oldrsp-136  currsp+16
 *                      8 bytes fileID
 * oldrsp-129  currsp+23
 *
 *
 * oldrsp-128
 *              128 bytes redzone
 * oldrsp-1
 *
 * oldrsp-0        caller(return) address
 */
void __attribute__((naked)) asmHookHandler() {
    //todo: Calculate values based on rsp rathe than saving them to registers
    __asm__ __volatile__ (


    //The first six integer or pointer arguments are passed in registers RDI, RSI, RDX, RCX, R8, R9
    // (R10 is used as a static chain pointer in case of nested functions[25]:21),
    //XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and XMM7  are used for the first floating point arguments
    //The wider YMM and ZMM registers

    //If the callee wishes to use registers RBX, RSP, RBP, and R12–R15, it must restore their original

    //Integer return values up to 64 bits in size are stored in RAX up to 128 bit are stored in RAX and RDX

    //For leaf-node functions (functions which do not call any other function(s)), a 128-byte space is stored just
    //beneath the stack pointer of the function. The space is called the red zone. This zone will not be clobbered
    //by any signal or interrupt handlers. Compilers can thus utilize this zone to save local variables.
    //gcc and clang offer the -mno-red-zone flag to disable red-zone optimizations.
    //todo: inefficient assembly code
    //R11 is the only register we can use. Store stack address in it. (%r11)==callerAddr. Later we'll pass this value
    //as a parameter to prehook
    /**
    * Save environment
    */
    "subq $0x80,%rsp\n\t"
    "movq %rax,(%rsp)\n\t" //8 bytes
    "movq %rcx,0x10(%rsp)\n\t" //8
    "movq %rdx,0x20(%rsp)\n\t" //8
    "movq %rsi,0x30(%rsp)\n\t" //8
    "movq %rdi,0x40(%rsp)\n\t" //8
    "movq %r8,0x50(%rsp)\n\t" //8
    "movq %r9,0x60(%rsp)\n\t" //8
    "movq %r10,0x70(%rsp)\n\t" //8

    //rsp%10h=0
    //        PUSHZMM(0) //16
    //        PUSHZMM(1) //16
    //        PUSHZMM(2) //16
    //        PUSHZMM(3) //16
    //        PUSHZMM(4) //16
    //        PUSHZMM(5) //16
    //        PUSHZMM(6) //16
    //        PUSHZMM(7) //16

    /**
     * Getting PLT entry address and caller address from stack
     */
    "movq 0x88(%rsp),%rdi\n\t"
    "movq 0x80(%rsp),%rsi\n\t"

    /**
     * Pre-Hook
     */
    //tips: Use http://www.sunshine2k.de/coding/javascript/onlineelfviewer/onlineelfviewer.html to find the external function name
    //todo: This is error on the server
    "call preHookHandler@plt\n\t"


    //Save return value to R11. This is the address of real function parsed by handler.
    //The return address is maybe the real function address. Or a pointer to the pseodoPlt table
    "movq %rax,%r11\n\t"

    "cmpq $1234,%rdi\n\t"
    "jnz  RET_FULL\n\t"

    //=======================================> if rdi==$1234
    "RET_PREHOOK_ONLY:\n\t"
    ASM_RESTORE_ENV_PREHOOK
    //Restore rsp to original value (Uncomment the following to only enable prehook)
    "addq $136,%rsp\n\t"
    "jmpq *%r11\n\t"

    //=======================================> if rdi!=$1234
    /**
     * Call actual function
     */
    "RET_FULL:\n\t"
    ASM_RESTORE_ENV_PREHOOK
    "addq $144,%rsp\n\t"
    "callq *%r11\n\t"

    /**
     * Save return value
     */
    //Save return value to stack
    "pushq %rax\n\t" //8
    "pushq %rdx\n\t" //8
    //                PUSHZMM(0) //16
    //                PUSHZMM(1) //16
    //Save st0
    //                "subq $16,%rsp\n\t" //16
    //                "fstpt (%rsp)\n\t"
    //                "subq $16,%rsp\n\t" //16
    //                "fstpt (%rsp)\n\t"

    /**
     * Call After Hook
     */
    //todo: This line has compilation error on the server
    "call afterHookHandler@plt\n\t"
    //Save return value to R11. R11 now has the address of caller.
    "movq %rax,%r11\n\t"

    /**
    * Restore return value
    */
    //                "fldt (%rsp)\n\t"
    //                "addq $16,%rsp\n\t" //16
    //                "fldt (%rsp)\n\t"
    //                "addq $16,%rsp\n\t" //16
    //                POPZMM(1) //16
    //                POPZMM(0) //16
    "popq %rdx\n\t" //8 (Used in afterhook)
    "popq %rax\n\t" //8


    /**
     * Restore callee saved register
     */
    //        "popq %r15\n\t" //8
    //        "popq %r14\n\t" //8
    //        "popq %r13\n\t" //8
    //        "popq %r12\n\t" //8
    //        "popq %rbp\n\t" //8
    //        "popq %rbx\n\t" //8
    //        "ldmxcsr (%rsp)\n\t" // 2 Bytes(8-4)
    //        "fldcw 4(%rsp)\n\t" // 4 Bytes(4-2)
    //        "addq $16,%rsp\n\t" //16


    //Recotre return address to the stack todo: Necessary?
    //"movq %r11,-8(%rsp)\n\t"


    //"CLD\n\t"
    //Retrun to caller
    "jmpq *%r11\n\t"
    );

}


//Return magic number definition:
// 1234: address resolved, pre-hook only (Fastest)
// else pre+afterhook. Check hook installation in afterhook
void *preHookHandler(uint64_t nextCallAddr, uint64_t fileId) {
    int32_t *callOperator = (int32_t *) (nextCallAddr - 4);
    uint8_t *pltEntryAddr = reinterpret_cast<uint8_t *>(nextCallAddr + *callOperator);

    scaler::ExtFuncCallHook *_this = scaler::ExtFuncCallHook::instance;

    scaler::ELFImgInfo &curElfImgInfo = _this->elfImgInfoMap.get(fileId);

    ssize_t pltEntryIndex;

    if (curElfImgInfo.pltSecStartAddr) {
        //Relative to plt
        pltEntryIndex = (pltEntryAddr - curElfImgInfo.pltSecStartAddr) / 16;
    } else {
        //Relative to pltsec
        pltEntryIndex = ((pltEntryAddr - curElfImgInfo.pltStartAddr) / 16) - 1;
    }
    scaler::SymID symbolId = curElfImgInfo.firstSymIndex + pltEntryIndex;
    scaler::ExtSymInfo &curElfSymInfo = _this->allExtSymbol.get(symbolId);


    //Starting from here, we could call external symbols and it won't cause any problem

    /**
     * No counting, no measuring time (If scaler is not installed, then tls is not initialized)
     * This may happen for external function call before the initTLS in dummy thread function
     */


    if (curContext == NULL) {
        //Skip afterhook
        asm volatile ("movq $1234, %%rdi" : /* No outputs. */
        :/* No inputs. */:"rdi");
        return curElfSymInfo.resolvedAddr != nullptr ? curElfSymInfo.resolvedAddr : curElfSymInfo.pltLdAddr;
    }

    if (bypassCHooks != SCALER_FALSE) {
        if (curElfSymInfo.resolvedAddr) {
            //Skip afterhook
            asm volatile ("movq $1234, %%rdi" : /* No outputs. */
            :/* No inputs. */:"rdi");
            return curElfSymInfo.resolvedAddr;
        } else {
            curContext->callerAddr.push((void *) nextCallAddr);
            curContext->fileId.push(fileId);
            curContext->symId.push(symbolId);
            return curElfSymInfo.pltLdAddr;
        }
    }

    bypassCHooks = SCALER_TRUE;

    //DBG_LOGS("FileId=%lu, pltId=%zd prehook", fileId, pltEntryIndex);

//    DBG_LOGS("[Pre Hook] Thread:%lu File(%ld):%s, Func(%ld): %s RetAddr:%p", pthread_self(),
//             curSymbol.fileId, _this->pmParser.idFileMap.at(curSymbol.fileId).c_str(),
//             extSymbolId, curSymbol.symbolName.c_str(), retOriFuncAddr);


    assert(curContext != nullptr);
//    if (curContextPtr->callerAddr.getSize() >= 2) {
//        //If exceeding the depth limit, there is no need to go further into after hook
//
//        //Skip afterhook (For debugging purpose)
//        asm volatile ("movq $1234, %%rdi"
//        : /* No outputs. */
//        :/* No inputs. */
//        :"rdi");
//        bypassCHooks = SCALER_FALSE;
//        return curElfSymInfo.resolvedAddr;
//    }

    /**
    * Setup environment for afterhook
    */
//    curContextPtr->timeStamp.push(getunixtimestampms());
    //Push callerAddr into stack
    curContext->callerAddr.push((void *) nextCallAddr);
    curContext->fileId.push(fileId);
    curContext->symId.push(symbolId);
    //Push calling info to afterhook
    //todo: rename this to caller function
    //curContextPtr->extSymbolId.push(extSymbolId);

    bypassCHooks = SCALER_FALSE;
    //printf("%p\n", curElfSymInfo.resolvedAddr);
    return curElfSymInfo.resolvedAddr ? curElfSymInfo.resolvedAddr : curElfSymInfo.pltLdAddr;
}

void *afterHookHandler() {
    bypassCHooks = SCALER_TRUE;
    scaler::ExtFuncCallHook *_this = scaler::ExtFuncCallHook::instance;
    HookContext *curContextPtr = curContext;
    assert(curContext != nullptr);


    scaler::SymID symbolId = curContextPtr->symId.peekpop();
    scaler::FileID fileId = curContextPtr->fileId.peekpop();
    void *callerAddr = curContextPtr->callerAddr.peekpop();
    scaler::ExtSymInfo &curElfSymInfo = _this->allExtSymbol[symbolId];

    if (curElfSymInfo.resolvedAddr == nullptr) {
        //Re-install hook
        curElfSymInfo.resolvedAddr = *curElfSymInfo.gotEntryAddr;
        switch (fileId) {
//            case 0:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper0);
//                break;
//            case 1:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper1);
//                break;
//            case 2:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper2);
//                break;
//            case 3:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper3);
//                break;
//            case 4:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper4);
//                break;
//            case 5:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper5);
//                break;
//            case 6:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper6);
//                break;
//            case 7:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper7);
//                break;
//            case 8:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper8);
//                break;
//            case 9:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper9);
//                break;
//            case 10:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper10);
//                break;
//            case 11:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper11);
//                break;
//            case 12:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper12);
//                break;
//            case 13:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper13);
//                break;
//            case 14:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper14);
//                break;
//            case 15:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper15);
//                break;
//            case 16:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper16);
//                break;
//            case 17:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper17);
//                break;
//            case 18:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper18);
//                break;
//            case 19:
//                *curElfSymInfo.gotEntryAddr = reinterpret_cast<uint8_t *>(redzoneJumper19);
//                break;
            default:
            fatalError("Impossible case");
        }
    }


//auto &funcName = curELFImgInfo.idFuncMap.at(extSymbolID)SymInfo.


//    const long long &preHookTimestamp = curContextPtr->timeStamp.peekpop();
////    DBG_LOGS("[After Hook] Thread ID:%lu Func(%ld) End: %llu",
////             pthread_self(), extSymbolID, getunixtimestampms() - preHookTimestamp);
//
//    //Resolve library id
//    scaler::ExtFuncCallHookAsm::ExtSymInfo &curSymbol = _this->allExtSymbol[extSymbolID];
//    if (curSymbol.libraryFileScalerID == -1) {
//        curSymbol.addr = *curSymbol.gotEntry;
//        assert(_this->isSymbolAddrResolved(curSymbol));
//        assert(curSymbol.addr != nullptr);
//        //Resolve address
//        curSymbol.libraryFileScalerID = _this->pmParser.findExecNameByAddr(curSymbol.addr);
//        assert(curSymbol.libraryFileScalerID != -1);
//    }
//
//    //Move
//
//
//    //Save this operation
//    const long long duration = getunixtimestampms() - preHookTimestamp;
//    if (curContextPtr->initialized == SCALER_TRUE) {
//        //Timing here
//    }
    bypassCHooks = SCALER_FALSE;
    return callerAddr;
}

void __attribute__((used, naked, noinline)) callLd() {
    __asm__ __volatile__ (
    "pushq $0\n\t"
    "movq $0xFFFFFFFFFFFFFFFF,%r11\n\t"
    "jmpq *%r11\n\t"
    );
}

void __attribute__((used, naked, noinline)) myPltEntry() {
    __asm__ __volatile__ (
    "movq $0x1122334455667788,%r11\n\t"
    "callq *%r11\n\t"
    );
}



