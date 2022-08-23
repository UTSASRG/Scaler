
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

#define likely(x) __builtin_expect(!!(x), 1) //x很可能为真
#define unlikely(x) __builtin_expect(!!(x), 0) //x很可能为假

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

#define ASM_SAVE_ENV_PREHOOK \
    "movq %rax,0x08(%rsp)\n\t" \
    "movq %rcx,0x18(%rsp)\n\t" \
    "movq %rdx,0x28(%rsp)\n\t" \
    "movq %rsi,0x38(%rsp)\n\t" \
    "movq %rdi,0x48(%rsp)\n\t" \
    "movq %r8,0x58(%rsp)\n\t" \
    "movq %r9,0x68(%rsp)\n\t" \
    "movq %r10,0x78(%rsp)\n\t"

#define ASM_RESTORE_ENV_PREHOOK \
    "movq 0x78(%rsp),%r10\n\t"\
    "movq 0x68(%rsp),%r9\n\t"\
    "movq 0x58(%rsp),%r8\n\t"\
    "movq 0x48(%rsp),%rdi\n\t"\
    "movq 0x38(%rsp),%rsi\n\t"\
    "movq 0x28(%rsp),%rdx\n\t"\
    "movq 0x18(%rsp),%rcx\n\t"\
    "movq 0x08(%rsp),%rax\n\t"


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
void __attribute__((naked)) asmTimingHandler() {
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
    ASM_SAVE_ENV_PREHOOK

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
    "movq (%rsp),%rsi\n\t" //First parameter, return addr
    "movq 0x118(%rsp),%rdi\n\t" //Second parameter, symbolId (Pushed to stack by idsaver)

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
    "addq $0x118,%rsp\n\t"
    "jmpq *%r11\n\t"

    //=======================================> if rdi!=$1234
    /**
     * Call actual function
     */
    "RET_FULL:\n\t"
    ASM_RESTORE_ENV_PREHOOK
    "addq $0x120,%rsp\n\t"
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


uint8_t *callIdSavers = nullptr;
uint8_t *ldCallers = nullptr;

//Return magic number definition:
//1234: address resolved, pre-hook only (Fastest)
//else pre+afterhook. Check hook installation in afterhook
__attribute__((used)) void *preHookHandler(uint64_t nextCallAddr, uint64_t symId) {
    HookContext *curContextPtr = curContext;
    void *retOriFuncAddr = nullptr;

    if (unlikely(curContextPtr == NULL)) {
        retOriFuncAddr = ldCallers + symId * 32;
        //Skip afterhook
        asm volatile ("movq $1234, %%rdi" : /* No outputs. */
        :/* No inputs. */:"rdi");
        return retOriFuncAddr;
    }

    //Assumes _this->allExtSymbol won't change
    scaler::ExtSymInfo &curElfSymInfo = curContextPtr->_this->allExtSymbol.internalArr[symId];

    if (unlikely(!curElfSymInfo.addrResolved)) {
        retOriFuncAddr = ldCallers + symId * 32;
    } else {
        retOriFuncAddr = *curElfSymInfo.gotEntryAddr;
    }

    /**
     * No counting, no measuring time (If scaler is not installed, then tls is not initialized)
     * This may happen for external function call before the initTLS in dummy thread function
     */

    if (unlikely(bypassCHooks == SCALER_TRUE)) {
        //Skip afterhook
        asm volatile ("movq $1234, %%rdi" : /* No outputs. */
        :/* No inputs. */:"rdi");
        return retOriFuncAddr;
    } else if (unlikely(curContextPtr->indexPosi >= MAX_CALL_DEPTH)) {
        //Skip afterhook
        asm volatile ("movq $1234, %%rdi" : /* No outputs. */
        :/* No inputs. */:"rdi");
        return retOriFuncAddr;
    } else if (unlikely((uint64_t) curContextPtr->hookTuple[curContextPtr->indexPosi].callerAddr == nextCallAddr)) {
        //Skip afterhook
        asm volatile ("movq $1234, %%rdi" : /* No outputs. */
        :/* No inputs. */:"rdi");
        return retOriFuncAddr;
    }

    bypassCHooks = SCALER_TRUE;

    //DBG_LOGS("FileId=%lu, pltId=%zd prehook", fileId, pltEntryIndex);

    DBG_LOGS("[Pre Hook] Thread:%lu CallerFileId:%ld Func:%ld RetAddr:%p Timestamp: %lu", pthread_self(),
             curElfSymInfo.fileId, symId, (void *) nextCallAddr, getunixtimestampms());
    //assert(curContext != nullptr);

    /**
    * Setup environment for afterhook
    */
    ++curContextPtr->indexPosi;

    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    curContextPtr->hookTuple[curContextPtr->indexPosi].timeStamp = ((int64_t) hi << 32) | lo;
    curContextPtr->hookTuple[curContextPtr->indexPosi].symId = symId;
    curContextPtr->hookTuple[curContextPtr->indexPosi].callerAddr = nextCallAddr;
    bypassCHooks = SCALER_FALSE;
    return retOriFuncAddr;
}

void *afterHookHandler() {
    bypassCHooks = SCALER_TRUE;
    HookContext *curContextPtr = curContext;
    assert(curContext != nullptr);


    scaler::SymID symbolId = curContextPtr->hookTuple[curContextPtr->indexPosi].symId;
    void *callerAddr = (void *) curContextPtr->hookTuple[curContextPtr->indexPosi].callerAddr;

    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    curContextPtr->recArr->internalArr[symbolId].timestamp +=
            (((int64_t) hi << 32) | lo) - curContextPtr->hookTuple[curContextPtr->indexPosi].timeStamp;

    --curContextPtr->indexPosi;
    assert(curContextPtr->indexPosi >= 1);

    scaler::ExtSymInfo &curElfSymInfo = curContextPtr->_this->allExtSymbol.internalArr[symbolId];

    if (unlikely(!curElfSymInfo.addrResolved)) {
        curElfSymInfo.addrResolved = true;
        //Resolve address
        //curElfSymInfo.libFileId = curContextPtr->_this->pmParser.findExecNameByAddr(*curElfSymInfo.gotEntryAddr);
        //assert(curElfSymInfo.libFileId != -1);
    }

    DBG_LOGS("[After Hook] Thread ID:%lu Func(%ld) CalleeFileId(%ld) Timestamp: %lu",
             pthread_self(), symbolId, curElfSymInfo.libFileId, getunixtimestampms());

    bypassCHooks = SCALER_FALSE;
    return callerAddr;
}

void __attribute__((used, naked, noinline)) myPltEntry() {
    __asm__ __volatile__ (
    "movq $0x1122334455667788,%r11\n\t"
    "jmpq *%r11\n\t"
    );
}

void __attribute__((used, naked, noinline)) callIdSaverScheme3() {
    __asm__ __volatile__ (
    /**
     * Access TLS, make sure it's initialized
     */
    "movabs $-32,%r11\n\t"//Move the tls offset of context to r11
    "mov %fs:(%r11),%r11\n\t" //Now r11 points to the tls header
    //Check whether the context is initialized
    "cmpq $0,%r11\n\t"
    //Skip processing if context is not initialized
    "jz SKIP\n\t"

    "pushq %r10\n\t"

    "movq 0x650(%r11),%r11\n\t" //Fetch recArr.internalArr address from TLS -> r11
    "movq 0x11223344(%r11),%r10\n\t" //Fetch recArr.internalArr[symId].count in Heap to -> r10
    "addq $1,%r10\n\t" //count + 1
    "movq %r10,0x11223344(%r11)\n\t" //Store count

    "movq 0x11223344(%r11),%r11\n\t" //Fetch recArr.internalArr[symId].gap in Heap to -> r11
    "andq %r11,%r10\n\t" //count value (r10) % gap (r11) -> r11, gap value must be a power of 2
    "cmpq $0,%r10\n\t" //If necessary count % gap == 0. Use timing
    "pop %r10\n\t"
    "jz TIMING_JUMPER\n\t" //Check if context is initialized

    /**
    * Return
    */
    "SKIP:"
    "movq $0x1122334455667788,%r11\n\t" //GOT address
    "jmpq (%r11)\n\t"
    "pushq $0x11223344\n\t" //Plt stub id
    "movq $0x1122334455667788,%r11\n\t" //First plt entry
    "jmpq *%r11\n\t"

    /**
     * Timing
     */
    //Perform timing
    "TIMING_JUMPER:"
    "pushq $0x11223344\n\t"
    "movq $0x1122334455667788,%r11\n\t"
    "jmpq *%r11\n\t"
    );
}