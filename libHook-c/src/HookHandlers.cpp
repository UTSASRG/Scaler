
#include <util/hook/HookHandlers.h>
#include <type/ExtSymInfo.h>
#include <util/hook/ExtFuncCallHook.h>
#include <util/hook/HookContext.h>
#include <util/tool/Timer.h>
#include <sys/times.h>

#define setbit(x, y) x|=(1<<y)
#define clrbit(x, y) x&=～(1<<y)
#define chkbit(x, y) x&(1<<y)


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


#define SAVE_COMPACT
#ifdef SAVE_ALL
#define SAVE_PRE  \
    /*The stack should be 16 bytes aligned before start this block*/ \
    /*ZMM registers are ignored. Normally we do not use them in out hook*/ \
    /*Parameter passing registers*/       \
    "movq %rax,0x08(%rsp)\n\t" /*8 bytes*/\
    "movq %rcx,0x10(%rsp)\n\t" /*8 bytes*/\
    "movq %rdx,0x18(%rsp)\n\t" /*8 bytes*/\
    "movq %rsi,0x20(%rsp)\n\t" /*8 bytes*/\
    "movq %rdi,0x28(%rsp)\n\t" /*8 bytes*/\
    "movq %r8,0x30(%rsp)\n\t"  /*8 bytes*/\
    "movq %r9,0x38(%rsp)\n\t"  /*8 bytes*/\
    "movq %r10,0x40(%rsp)\n\t" /*8 bytes*/\
    /*Call-ee saved registers */ \
    "movq %rbx,0x48(%rsp)\n\t" /*8 bytes*/\
    "movq %rbp,0x50(%rsp)\n\t" /*8 bytes*/\
    "movq %r12,0x58(%rsp)\n\t" /*8 bytes*/\
    "movq %r13,0x60(%rsp)\n\t" /*8 bytes*/\
    "movq %r14,0x68(%rsp)\n\t" /*8 bytes*/\
    "movq %r15,0x70(%rsp)\n\t" /*8 bytes*/\
    "stmxcsr 0x78(%rsp)\n\t" /*16bytes*/  \
    "fstcw 0xEA(%rsp)\n\t"                \
    /*The following addr must be 16 bits aligned*/ \
    /*Save XMM1 to XMM7*/                          \
    "vmovdqu64 %zmm0,0x88(%rsp) \n\t"/*64bytes*/  \
    "vmovdqu64 %zmm1,0xC8(%rsp) \n\t"/*64bytes*/  \
    "vmovdqu64 %zmm2,0x108(%rsp) \n\t"/*64bytes*/  \
    "vmovdqu64 %zmm3,0x148(%rsp) \n\t"/*64bytes*/  \
    "vmovdqu64 %zmm4,0x188(%rsp) \n\t"/*64bytes*/  \
    "vmovdqu64 %zmm5,0x1C8(%rsp) \n\t"/*64bytes*/  \
    "vmovdqu64 %zmm6,0x208(%rsp) \n\t"/*64bytes*/  \
    "vmovdqu64 %zmm7,0x248(%rsp) \n\t"/*64bytes*/  \

#define SAVE_BYTES_PRE "0x288" //0x248+512
#define SAVE_BYTES_PRE_plus8 "0x290" //0x288+0x8
#define SAVE_BYTES_PRE_plus16 "0x298" //0x288+0x8

//Do not write restore by yourself, copy previous code and reverse operand order
#define RESTORE_PRE  \
    "movq 0x08(%rsp),%rax\n\t" /*8 bytes*/\
    "movq 0x10(%rsp),%rcx\n\t" /*8 bytes*/\
    "movq 0x18(%rsp),%rdx\n\t" /*8 bytes*/\
    "movq 0x20(%rsp),%rsi\n\t" /*8 bytes*/\
    "movq 0x28(%rsp),%rdi\n\t" /*8 bytes*/\
    "movq 0x30(%rsp),%r8\n\t"  /*8 bytes*/\
    "movq 0x38(%rsp),%r9\n\t"  /*8 bytes*/\
    "movq 0x40(%rsp),%r10\n\t" /*8 bytes*/\
    /*Call-ee saved registers */ \
    "movq 0x48(%rsp),%rbx\n\t" /*8 bytes*/\
    "movq 0x50(%rsp),%rbp\n\t" /*8 bytes*/\
    "movq 0x58(%rsp),%r12\n\t" /*8 bytes*/\
    "movq 0x60(%rsp),%r13\n\t" /*8 bytes*/\
    "movq 0x68(%rsp),%r14\n\t" /*8 bytes*/\
    "movq 0x70(%rsp),%r15\n\t" /*8 bytes*/\
    "ldmxcsr 0x78(%rsp)\n\t" /*16bytes*/  \
    "fldcw 0xEA(%rsp)\n\t"                \
    /*The following addr must be 16 bits aligned*/ \
    /*Save XMM1 to XMM7*/                          \
    "vmovdqu64 0x88(%rsp),%zmm0 \n\t"/*512bytes*/  \
    "vmovdqu64 0xC8(%rsp),%zmm1 \n\t"/*512bytes*/  \
    "vmovdqu64 0x108(%rsp),%zmm2 \n\t"/*512bytes*/  \
    "vmovdqu64 0x148(%rsp),%zmm3 \n\t"/*512bytes*/  \
    "vmovdqu64 0x188(%rsp),%zmm4 \n\t"/*512bytes*/  \
    "vmovdqu64 0x1C8(%rsp),%zmm5 \n\t"/*512bytes*/  \
    "vmovdqu64 0x208(%rsp),%zmm6 \n\t"/*512bytes*/  \
    "vmovdqu64 0x248(%rsp),%zmm7 \n\t"/*512bytes*/  \


#define SAVE_POST  \
    /*The stack should be 16 bytes aligned before start this block*/       \
    /*ZMM registers are ignored. Normally we do not use them in out hook*/ \
    /*Parameter passing registers*/                                        \
    "movq %rax,(%rsp)\n\t" /*8 bytes*/                                     \
    "movq %rdx,0x8(%rsp)\n\t" /*8 bytes*/                                  \
    "vmovdqu64 %zmm0,0x10(%rsp) \n\t"/*64bytes*/                           \
    "vmovdqu64 %zmm1,0x50(%rsp) \n\t"/*64bytes*/                           \
    /*https://www.cs.mcgill.ca/~cs573/winter2001/AttLinux_syntax.htm*/     \
    "fsave 0x90(%rsp)\n\t" /*108bytes*/                                              \

#define SAVE_BYTES_POST "0xFC" /*0x90+108*/


#define RESTORE_POST  \
    /*Parameter passing registers*/                                        \
    "movq (%rsp),%rax\n\t" /*8 bytes*/                                     \
    "movq 0x8(%rsp),%rdx\n\t" /*8 bytes*/                                  \
    "vmovdqu64 0x10(%rsp),%zmm0 \n\t"/*64bytes*/                           \
    "vmovdqu64 0x50(%rsp),%zmm1 \n\t"/*64bytes*/                           \
    /*https://www.cs.mcgill.ca/~cs573/winter2001/AttLinux_syntax.htm*/     \
    "fnsave 0x90(%rsp)\n\t" /*108bytes*/
#endif

//In this mode, we reduce registers that is not used.
#ifdef SAVE_COMPACT
#define SAVE_PRE  \
    /*The stack should be 16 bytes aligned before start this block*/ \
    /*ZMM registers are ignored. Normally we do not use them in out hook*/ \
    /*Parameter passing registers*/       \
    "movq %rax,0x08(%rsp)\n\t" /*8 bytes*/\
    "movq %rcx,0x10(%rsp)\n\t" /*8 bytes*/\
    "movq %rdx,0x18(%rsp)\n\t" /*8 bytes*/\
    "movq %rsi,0x20(%rsp)\n\t" /*8 bytes*/\
    "movq %rdi,0x28(%rsp)\n\t" /*8 bytes*/\
    "movq %r8,0x30(%rsp)\n\t"  /*8 bytes*/\
    "movq %r9,0x38(%rsp)\n\t"  /*8 bytes*/\
    "movq %r10,0x40(%rsp)\n\t" /*8 bytes*/\

#define SAVE_BYTES_PRE "0x48" //0x40+8
#define SAVE_BYTES_PRE_plus8 "0x50" //0x40+0x8
#define SAVE_BYTES_PRE_plus16 "0x58" //0x40+0x10

//Do not write restore by yourself, copy previous code and reverse operand order
#define RESTORE_PRE  \
    "movq 0x08(%rsp),%rax\n\t" /*8 bytes*/\
    "movq 0x10(%rsp),%rcx\n\t" /*8 bytes*/\
    "movq 0x18(%rsp),%rdx\n\t" /*8 bytes*/\
    "movq 0x20(%rsp),%rsi\n\t" /*8 bytes*/\
    "movq 0x28(%rsp),%rdi\n\t" /*8 bytes*/\
    "movq 0x30(%rsp),%r8\n\t"  /*8 bytes*/\
    "movq 0x38(%rsp),%r9\n\t"  /*8 bytes*/\
    "movq 0x40(%rsp),%r10\n\t" /*8 bytes*/\



//#define SAVE_POST  \
//    /*The stack should be 16 bytes aligned before start this block*/       \
//    /*ZMM registers are ignored. Normally we do not use them in out hook*/ \
//    /*Parameter passing registers*/                                        \
//    "movq %rax,(%rsp)\n\t" /*8 bytes*/                                     \
//    "movq %rdx,0x8(%rsp)\n\t" /*8 bytes*/                                  \
//
//#define SAVE_BYTES_POST "0x90" /*0x8+0x8+0x80*/
//
//#define RESTORE_POST  \
//    /*Parameter passing registers*/                                        \
//    "movq (%rsp),%rax\n\t" /*8 bytes*/                                     \
//    "movq 0x8(%rsp),%rdx\n\t" /*8 bytes*/                                  \

#define SAVE_POST  \
    /*The stack should be 16 bytes aligned before start this block*/       \
    /*ZMM registers are ignored. Normally we do not use them in out hook*/ \
    /*Parameter passing registers*/                                        \
    "movq %rax,(%rsp)\n\t" /*8 bytes*/                                     \
    "movq %rdx,0x8(%rsp)\n\t" /*8 bytes*/                                  \
    "movdqu %xmm0,0x10(%rsp) \n\t"/*16bytes*/                              \
    "movdqu %xmm1,0x20(%rsp) \n\t"/*16bytes*/                            \
    /*https://www.cs.mcgill.ca/~cs573/winter2001/AttLinux_syntax.htm*/     \
    /*"fsave 0x10(%rsp)\n\t"*/ /*108bytes*/                                              \

#define SAVE_BYTES_POST "0x30" /*0x20+18*/


#define RESTORE_POST  \
    /*Parameter passing registers*/                                        \
    "movq (%rsp),%rax\n\t" /*8 bytes*/                                     \
    "movq 0x8(%rsp),%rdx\n\t" /*8 bytes*/                                  \
    "movdqu 0x10(%rsp),%xmm0 \n\t"/*64bytes*/                           \
    "movdqu 0x20(%rsp),%xmm1 \n\t"/*64bytes*/                           \
    /*https://www.cs.mcgill.ca/~cs573/winter2001/AttLinux_syntax.htm*/     \
    /*"fnsave 0x10(%rsp)\n\t"*/ /*108bytes*/

#endif


/**
 * Source code version for #define IMPL_ASMHANDLER
 * We can't add comments to a macro
 */
void __attribute__((naked)) asmTimingHandler() {
    //todo: Calculate values based on rsp rathe than saving them to registers
    __asm__ __volatile__ (
    /**
     * Reserve stack space
     */
    "subq $" SAVE_BYTES_PRE ",%rsp\n\t" //rsp -= SAVE_BYTES_PRE

    /**
    * Save environment
    */
    SAVE_PRE

    /**
     * Getting PLT entry address and caller address from stack
     */
    "movq " SAVE_BYTES_PRE_plus8 "(%rsp),%rdi\n\t" //First parameter, return addr
    "movq " SAVE_BYTES_PRE "(%rsp),%rsi\n\t" //Second parameter, symbolId (Pushed to stack by idsaver)

    /**
     * Pre-Hook
     */
    "call preHookHandler@plt\n\t"

    //Save return value to R11. This is the address of real function parsed by handler.
    //The return address is maybe the real function address. Or a pointer to the pseodoPlt table
    "movq %rax,%r11\n\t"
    "cmpq $1234,%rdi\n\t"
    "jz  RET_PREHOOK_ONLY\n\t"

    //=======================================> if rdi!=$1234
    /**
     * Call actual function
     */
    RESTORE_PRE
    "addq $" SAVE_BYTES_PRE_plus16 ",%rsp\n\t" //Plus 8 is because there was a push to save 8 bytes more funcId. Another 8 is to replace return address
    "callq *%r11\n\t"

    /**
     * Call after hook
     */
    //Save return value to stack
    "subq $" SAVE_BYTES_POST ",%rsp\n\t"
    SAVE_POST

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
    RESTORE_POST

    //Retrun to caller
    "addq $" SAVE_BYTES_POST ",%rsp\n\t" //Plus 8 is because there was a push to save 8 bytes more funcId. Another 8 is to replace return address
    "jmpq *%r11\n\t"


    //=======================================> if rdi==$1234
    "RET_PREHOOK_ONLY:\n\t"
    RESTORE_PRE
    //Restore rsp to original value (Uncomment the following to only enable prehook)
    "addq $" SAVE_BYTES_PRE_plus8 ",%rsp\n\t" //Plus 8 is because there was a push to save 8 bytes more funcId
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

    //Assumes _this->allExtSymbol won't change
    scaler::ExtSymInfo &curElfSymInfo = curContextPtr->_this->allExtSymbol.internalArr[symId];
    void *retOriFuncAddr = *curElfSymInfo.gotEntryAddr;

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

//    INFO_LOGS("[Pre Hook] Thread:%lu CallerFileId:%ld Func:%ld RetAddr:%p Timestamp: %lu\n", pthread_self(),
//             curElfSymInfo.fileId, symId, (void *) nextCallAddr, getunixtimestampms());
    //assert(curContext != nullptr);

    /**
    * Setup environment for afterhook
    */
    ++curContextPtr->indexPosi;
    int64_t &c = curContextPtr->recArr->internalArr[symId].count;
//    if (c < (1 << 10)) {
//        struct tms curTime;
//        times(&curTime);
//        curContextPtr->hookTuple[curContextPtr->indexPosi].clockTicks = curTime.tms_utime + curTime.tms_stime;
//        printf("Clock ticks in prehook=%d\n",curContextPtr->hookTuple[curContextPtr->indexPosi].clockTicks);
//    }
    curContextPtr->hookTuple[curContextPtr->indexPosi].symId = symId;
    curContextPtr->hookTuple[curContextPtr->indexPosi].callerAddr = nextCallAddr;
    bypassCHooks = SCALER_FALSE;
    curContextPtr->hookTuple[curContextPtr->indexPosi].clockCycles = getunixtimestampms();
    return *curElfSymInfo.gotEntryAddr;
}

__attribute__((used)) void *dbgPreHandler(uint64_t nextCallAddr, uint64_t symId) {
    HookContext *curContextPtr = curContext;

    //Assumes _this->allExtSymbol won't change
    scaler::ExtSymInfo &curElfSymInfo = curContextPtr->_this->allExtSymbol.internalArr[symId];
    void *retOriFuncAddr = *curElfSymInfo.gotEntryAddr;


    asm volatile ("movq $0x11223344, %%rax\n\t"
                  "movq $0x11223344, %%rcx\n\t"
                  "movq $0x11223344, %%rdx\n\t"
                  "movq $0x11223344, %%rsi\n\t"
                  "movq $0x11223344, %%rdi\n\t"
                  "movq $0x11223344, %%r8\n\t"
                  "movq $0x11223344, %%r9\n\t"
                  "movq $0x11223344, %%r10\n\t"
                  "movq $0x11223344, %%rbx\n\t"
                  "movq $0x11223344, %%rbp\n\t"
                  "movq $0x11223344, %%r12\n\t"
                  "movq $0x11223344, %%r13\n\t"
                  "movq $0x11223344, %%r14\n\t"
                  "movq $0x11223344, %%r15\n\t"

                  "vmovdqu64 (%%rsp), %%zmm0\n\t"
                  "vmovdqu64 (%%rsp), %%zmm1\n\t"
                  "vmovdqu64 (%%rsp), %%zmm2\n\t"
                  "vmovdqu64 (%%rsp), %%zmm3\n\t"
                  "vmovdqu64 (%%rsp), %%zmm4\n\t"
                  "vmovdqu64 (%%rsp), %%zmm5\n\t"
                  "vmovdqu64 (%%rsp), %%zmm6\n\t"
                  "vmovdqu64 (%%rsp), %%zmm7\n\t"
    : /* No outputs. */
    :/* No inputs. */
    :"rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "rbx", "rbp", "r12", "r13", "r14", "r15", "zmm0", "zmm1", "zmm3", "zmm4", "zmm5", "zmm6", "zmm7");

    //Skip afterhook
    asm volatile ("movq $1234, %%rdi" : /* No outputs. */
    :/* No inputs. */:"rdi");
    return retOriFuncAddr;
}

void *afterHookHandler() {
    uint64_t postHookClockCycles = getunixtimestampms();
    bypassCHooks = SCALER_TRUE;
    HookContext *curContextPtr = curContext;
    assert(curContext != nullptr);

    scaler::SymID symbolId = curContextPtr->hookTuple[curContextPtr->indexPosi].symId;
    void *callerAddr = (void *) curContextPtr->hookTuple[curContextPtr->indexPosi].callerAddr;

//    int64_t prevClockTick = curContextPtr->hookTuple[curContextPtr->indexPosi].clockTicks;
    uint64_t preClockCycle = curContextPtr->hookTuple[curContextPtr->indexPosi].clockCycles;


    int64_t &c = curContextPtr->recArr->internalArr[symbolId].count;


    --curContextPtr->indexPosi;
    assert(curContextPtr->indexPosi >= 1);

    scaler::ExtSymInfo &curElfSymInfo = curContextPtr->_this->allExtSymbol.internalArr[symbolId];

    //compare current timestamp with the previous timestamp

    float &meanClockCycle = curContextPtr->recArr->internalArr[symbolId].meanClockTick;
    int32_t &clockCycleThreshold = curContextPtr->recArr->internalArr[symbolId].durThreshold;

    int64_t clockCyclesDuration = (int64_t) (postHookClockCycles - preClockCycle);

    if(threadNum>1){
        clockCyclesDuration=clockCyclesDuration/threadNum;
    }

#ifdef INSTR_TIMING
    TIMING_TYPE &curSize = detailedTimingVectorSize[symbolId];
    if (curSize < TIMING_REC_COUNT) {
        ++curSize;
        detailedTimingVectors[symbolId][curSize] = clockCyclesDuration;
    }
#endif

    //RDTSCTiming if not skipped
    curContextPtr->recArr->internalArr[symbolId].totalClockCycles += clockCyclesDuration;

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
    "mov $0x1122334455667788,%r11\n\t"//Move the tls offset of context to r11
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
    "jmpq *(%r11)\n\t"
    "pushq $0x11223344\n\t" //Plt stub id
    "movq $0x1122334455667788,%r11\n\t" //First plt entry
    "jmpq *%r11\n\t"

    /**
     * Timing
     */
    //Perform timing
    "TIMING_JUMPER:"
    "pushq $0x11223344\n\t" //Save funcId to stack
    "movq $0x1122334455667788,%r11\n\t"
    "jmpq *%r11\n\t"
    );
}


void __attribute__((used, naked, noinline)) callIdSaverScheme4() {
    __asm__ __volatile__ (
    /**
     * Read TLS part
     */
    "mov $0x1122334455667788,%r11\n\t"//Move the tls offset of context to r11
    "mov %fs:(%r11),%r11\n\t" //Now r11 points to the tls header
    //Check whether the context is initialized
    "cmpq $0,%r11\n\t"
    //Skip processing if context is not initialized
    "jz SKIP1\n\t"


    /**
     * Counting part
     */
    "pushq %r10\n\t"

    "movq 0x650(%r11),%r11\n\t" //Fetch recArr.internalArr address from TLS -> r11
    "movq 0x11223344(%r11),%r10\n\t" //Fetch recArr.internalArr[symId].count in Heap to -> r10
    "addq $1,%r10\n\t" //count + 1
    "movq %r10,0x11223344(%r11)\n\t" //Store count

    "movq 0x11223344(%r11),%r11\n\t" //Fetch recArr.internalArr[symId].gap in Heap to -> r11
    "andq %r11,%r10\n\t" //count value (r10) % gap (r11) -> r11, gap value must be a power of 2
    "cmpq $0,%r10\n\t" //If necessary count % gap == 0. Use timing
    "pop %r10\n\t"
    "jz TIMING_JUMPER1\n\t" //Check if context is initialized

    /**
    * Return
    */
    "SKIP1:"
    "movq $0x1122334455667788,%r11\n\t" //GOT address
    "jmpq *(%r11)\n\t"
    "pushq $0x11223344\n\t" //Plt stub id
    "movq $0x1122334455667788,%r11\n\t" //First plt entry
    "jmpq *%r11\n\t"

    /**
     * Timing
     */
    //Perform timing
    "TIMING_JUMPER1:"
    "pushq $0x11223344\n\t" //Save funcId to stack
    "movq $0x1122334455667788,%r11\n\t"
    "jmpq *%r11\n\t"
    );
}