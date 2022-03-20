#include <plthook.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <link.h>
#include <cstring>
#include <vector>
#include <map>
#include <libtest.h>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

//https://stackoverflow.com/questions/10161911/push-xmm-register-to-the-stack

#define PUSHXMM(ArgumentName) \
"subq $16,%rsp\n\t" \
"movdqu  %xmm"#ArgumentName" ,(%rsp)\n\t"

#define POPXMM(ArgumentName) \
"movdqu  (%rsp),%xmm"#ArgumentName"\n\t"\
"addq $16,%rsp\n\t"


#define PUSH64bits(ArgumentName) \
"subq $8,%rsp\n\t" \
"movdqu  %"#ArgumentName",(%rsp)\n\t"

#define POP64bits(ArgumentName) \
"movdqu  (%rsp),%"#ArgumentName"\n\t"\
"addq $16,%rsp\n\t"

#define ALIGN_ADDR(addr, page_size) (void *) ((size_t) (addr) / page_size * page_size)

extern char __startpltgot, __startplt, __startpltsec;
extern char *ptrPltgot = &__startpltgot, *ptrPlt = &__startplt, *ptrPltSec = &__startpltsec;

using namespace std;

bool *realAddrResolved = nullptr;
const char **hookedNames = nullptr;
void **hookedAddrs = nullptr;
int *hookedIds = nullptr;
char *oriPltOpCodesArr = nullptr;
long hookedCount = 0;

plthook_t *myPltHook = nullptr;
thread_local static bool SCALER_HOOK_IN_HOOK_HANDLER = false;

extern "C" {

void *cHookHanlder(int index, void *funcAddr) {
//    printf("cHookHanlder funcAddr:%p\n", funcAddr);
    funcAddr = reinterpret_cast<void *>(uint64_t(funcAddr) - 0xD);
    int funcId = ((uint64_t) funcAddr - (uint64_t) ptrPlt) / 16 - 1;
//    printf("CHookHandler %s is called\n", hookedNames[funcId]);

    if (!realAddrResolved[funcId]) {
        void *curGotFuncAddr = plthook_GetPltAddress(&myPltHook, funcId);
        if (hookedAddrs[funcId] != curGotFuncAddr) {
//            printf("Address is now resolved，replace hookedAddrs with correct value\n");
            //Update address and set it as correct address
            realAddrResolved[funcId] = true;
            hookedAddrs[funcId] = curGotFuncAddr;
        } else {
//            printf("FuncA is not initialized, execute orignal PLT code\n");
            //Execute original PLT function
            return oriPltOpCodesArr + funcId * 16;
        }
    }

    return hookedAddrs[funcId];
}

void *cHookHanlderSec(int index, void *funcAddr) {
    pthread_mutex_lock()


    funcAddr = reinterpret_cast<void *>(uint64_t(funcAddr) - 0xD);
    int funcId = ((uint64_t) funcAddr - (uint64_t) ptrPltSec) / 16;

    if (!realAddrResolved[funcId]) {
        void *curGotFuncAddr = plthook_GetPltAddress(&myPltHook, funcId);
        if (hookedAddrs[funcId] != curGotFuncAddr) {
//            printf("Address is now resolved，replace hookedAddrs with correct value\n");
            //Update address and set it as correct address
            realAddrResolved[funcId] = true;
            hookedAddrs[funcId] = curGotFuncAddr;
            return hookedAddrs[funcId];
        } else {
//            printf("%s is not initialized, execute orignal PLT code\n", hookedNames[funcId]);
            SCALER_HOOK_IN_HOOK_HANDLER = false;
            //Execute original PLT function
            return oriPltOpCodesArr + funcId * 18;
        }
    }

    if (SCALER_HOOK_IN_HOOK_HANDLER) {
        return hookedAddrs[funcId];
    }

    SCALER_HOOK_IN_HOOK_HANDLER = true;
    malloc(1);
    malloc(2);
    malloc(3);
//    printf("Here I am!!! %d", 1);
    SCALER_HOOK_IN_HOOK_HANDLER = false;
    return hookedAddrs[funcId];
}


void cHookHanlderAfter() {
    //printf("After Hook \n");
}

}

void __attribute__((naked)) demoFunc() {
    __asm__ __volatile__ (
    "movq  $0xFFFFFFFF,%r11\n\t"
    "call *%r11\n\t"
    );

}

void __attribute__((naked)) demoFunc1() {
    __asm__ __volatile__ (
    "push $0xFFFF\n\t"
    "movq $0xFFFFFFFF,%r11\n\t"
    "jmp *%r11\n\t"
    );

}

void __attribute__((naked)) demoFunc2() {
    __asm__ __volatile__ (
    "movq  $0xFFFFFFFF,%r11\n\t"
    "jmp *%r11\n\t"
    );

}

void __attribute__((naked)) asmHookHandler() {
    __asm__ __volatile__ (
    "popq %r15\n\t" //SaveReturnAddress 离头差了D

    "push %rdi\n\t"
    "push %rsi\n\t"
    "push %rdx\n\t"
    "push %rcx\n\t"
    "push %r8\n\t"
    "push %r9\n\t"
    PUSHXMM(0)
    PUSHXMM(1)
    PUSHXMM(2)
    PUSHXMM(3)
    PUSHXMM(4)
    PUSHXMM(5)
    PUSHXMM(6)
    PUSHXMM(7)

    "movq %r15,%rsi\n\t"
    "movq $1,%rdi\n\t"
    "call  cHookHanlder\n\t"
    "movq %rax,%r15\n\t"

    POPXMM(7)
    POPXMM(6)
    POPXMM(5)
    POPXMM(4)
    POPXMM(3)
    POPXMM(2)
    POPXMM(1)
    POPXMM(0)
    "pop %r9\n\t"
    "pop %r8\n\t"
    "pop %rcx\n\t"
    "pop %rdx\n\t"
    "pop %rsi\n\t"
    "pop %rdi\n\t"

    "jmp *%r15\n\t"
    "ret\n\t"
    );
}

void __attribute__((naked)) asmHookHandlerSec() {
    __asm__ __volatile__ (
    "popq %r14\n\t" //SaveReturnAddress 离头差了D
    "popq %r13\n\t" //SaveReturnAddress 离头差了D

    "push %rdi\n\t"
    "push %rsi\n\t"
    "push %rdx\n\t"
    "push %rcx\n\t"
    "push %r8\n\t"
    "push %r9\n\t"
    PUSHXMM(0)
    PUSHXMM(1)
    PUSHXMM(2)
    PUSHXMM(3)
    PUSHXMM(4)
    PUSHXMM(5)
    PUSHXMM(6)
    PUSHXMM(7)

    "movq %r14,%rsi\n\t"
    "movq $1,%rdi\n\t"
    "call  cHookHanlderSec\n\t"
    "movq %rax,%r14\n\t"

    POPXMM(7)
    POPXMM(6)
    POPXMM(5)
    POPXMM(4)
    POPXMM(3)
    POPXMM(2)
    POPXMM(1)
    POPXMM(0)
    "pop %r9\n\t"
    "pop %r8\n\t"
    "pop %rcx\n\t"
    "pop %rdx\n\t"
    "pop %rsi\n\t"
    "pop %rdi\n\t"

    "call *%r14\n\t" //Call original function

    "call  cHookHanlderAfter\n\t" //Call after hook

    "jmp *%r13\n\t"  //Jump to caller
    );
}

void fillDestAddrIntoHookCode(uint8_t *binCodeArr, uint64_t funcAddr) {
//    uint8_t replacement[] = {73, 187, 00, 00, 00, 00, 00, 00, 00, 00, 65, 255, 227};
    const uint64_t h1 = 0b00000000000000000000000011111111;
    const uint64_t h2 = h1 << 8;
    const uint64_t h3 = h1 << 16;
    const uint64_t h4 = h1 << 24;
    const uint64_t h5 = h1 << 32;
    const uint64_t h6 = h1 << 40;
    const uint64_t h7 = h1 << 48;
    const uint64_t h8 = h1 << 56;
//    printf("%d\n", funcAddr & h1);
//    printf("%d\n", (funcAddr & h2) >> 8);
//    printf("%d\n", (funcAddr & h3) >> 16);
//    printf("%d\n", (funcAddr & h4) >> 24);
//    printf("%d\n", (funcAddr & h5) >> 32);
//    printf("%d\n", (funcAddr & h6) >> 40);
//    printf("%d\n", (funcAddr & h7) >> 48);
//    printf("%d\n", (funcAddr & h8) >> 56);
    binCodeArr[2] = funcAddr & h1;
    binCodeArr[3] = (funcAddr & h2) >> 8;
    binCodeArr[4] = (funcAddr & h3) >> 16;
    binCodeArr[5] = (funcAddr & h4) >> 24;
    binCodeArr[6] = (funcAddr & h5) >> 32;
    binCodeArr[7] = (funcAddr & h6) >> 40;
    binCodeArr[8] = (funcAddr & h7) >> 48;
    binCodeArr[9] = (funcAddr & h8) >> 56;
}

void fillDestAddrFuncIDIntoPltCode(uint8_t *binCodeArr, uint8_t funcId, uint64_t funcAddr) {
//    uint8_t replacement[] = {104, 00, 00, 00, 00, 73, 191, 00, 00, 00, 00, 00, 00, 00, 00, 65, 255, 231};

    const uint64_t h1 = 0b00000000000000000000000011111111;
    const uint64_t h2 = h1 << 8;
    const uint64_t h3 = h1 << 16;
    const uint64_t h4 = h1 << 24;
    const uint64_t h5 = h1 << 32;
    const uint64_t h6 = h1 << 40;
    const uint64_t h7 = h1 << 48;
    const uint64_t h8 = h1 << 56;

    binCodeArr[1] = funcId & h1;
    binCodeArr[2] = (funcId & h2) >> 8;
    binCodeArr[3] = (funcId & h3) >> 16;
    binCodeArr[4] = (funcId & h4) >> 24;


    binCodeArr[7] = funcAddr & h1;
    binCodeArr[8] = (funcAddr & h2) >> 8;
    binCodeArr[9] = (funcAddr & h3) >> 16;
    binCodeArr[10] = (funcAddr & h4) >> 24;
    binCodeArr[11] = (funcAddr & h5) >> 32;
    binCodeArr[12] = (funcAddr & h6) >> 40;
    binCodeArr[13] = (funcAddr & h7) >> 48;
    binCodeArr[14] = (funcAddr & h8) >> 56;
}


void enumPltNames() {
    unsigned int pos = 0;
    const char *name;
    void **addr;
    int i;
    printf("===>\n", name);
    while (plthook_enum(myPltHook, &pos, &name, &addr) == 0) {
        printf("%d:%s\n", pos - 1, name);
    }
    printf("<===\n", name);

}

void hookPLTSEC() {
    //Find plthook
    plthook_open(&myPltHook, NULL);

    //Print all functions and it's index
    enumPltNames();


    unsigned int pos = 0;
    const char *name;
    void **addr;
    int i;

    hookedNames = static_cast<const char **>(malloc(myPltHook->rela_plt_cnt * sizeof(const char *)));
    hookedAddrs = static_cast<void **>(malloc(myPltHook->rela_plt_cnt * sizeof(void *)));
    realAddrResolved = static_cast<bool *>(malloc(myPltHook->rela_plt_cnt * sizeof(bool)));
    hookedIds = static_cast<int *>(malloc(myPltHook->rela_plt_cnt * sizeof(int)));
    oriPltOpCodesArr = static_cast<char *>(malloc(myPltHook->rela_plt_cnt * 18));

    printf("oriPltOpCodesArr: %p\n", oriPltOpCodesArr);
    static size_t page_size = sysconf(_SC_PAGESIZE);
    //Give execution permission to this part of memory
    int retCode = mprotect(ALIGN_ADDR(oriPltOpCodesArr, page_size), myPltHook->rela_plt_cnt * 18,
                           PROT_READ | PROT_WRITE | PROT_EXEC);
    if (retCode != 0) {
        printf("Could not change the process memory permission at %p\n", ALIGN_ADDR(ptrPlt, page_size));
    }

    while (plthook_enum(myPltHook, &pos, &name, &addr) == 0) {
        int result = strncmp(name, "__printf", 8);
        if (result == 0) {
            printf("%s Hooked: %p\n", name, addr);
            hookedNames[pos - 1] = name;
            hookedAddrs[pos - 1] = *addr;
            realAddrResolved[pos - 1] = true;
            hookedIds[hookedCount] = pos - 1;
            ++hookedCount;
        }
    }

    uint8_t templateHookCode[] = {73, 187, 00, 00, 00, 00, 00, 00, 00, 00, 65, 255, 227, 104, 144, 144};

    uint8_t templatePLT[] = {104, 00, 00, 00, 00, 73, 191, 00, 00, 00, 00, 00, 00, 00, 00, 65, 255, 231};

    for (int i = 0; i < hookedCount; ++i) {
        //Replace PLT with inline hook code
//        fillDestAddrIntoHookCode(templateHookCode, (uint64_t) asmHookHandler);
        memcpy(ptrPlt + 16 * (hookedIds[i] + 1), templateHookCode, 16);

        //Replace PLT.SEC with inline hook code
        fillDestAddrIntoHookCode(templateHookCode, (uint64_t) asmHookHandlerSec);
        memcpy(ptrPltSec + 16 * (hookedIds[i]), templateHookCode, 16);

        //Replace Heap PLT with psedo PLT code
        fillDestAddrFuncIDIntoPltCode(templatePLT, hookedIds[i], (uint64_t) ptrPlt);
        memcpy(oriPltOpCodesArr + 18 * (hookedIds[i]), templatePLT, 18);
//        printf("oriPltOpCodesArr + 18 * (hookedIds[i])=%p\n", oriPltOpCodesArr + 18 * (hookedIds[i]));


//        for (int i = 0; i < 16; ++i) {
//            printf("%d, ", templateHookCode[i]);
//        }
//        printf("\n");
    }

}


__attribute__((constructor)) void libConstructor() {
    //Install Hook
    printf("libConstructor\n");

    //Remove protection
    static size_t page_size = sysconf(_SC_PAGESIZE);

    if (mprotect(ALIGN_ADDR(ptrPlt, page_size), page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        printf("Could not change the process memory permission at %p\n", ALIGN_ADDR(ptrPlt, page_size));
    }
    if (mprotect(ALIGN_ADDR(ptrPltgot, page_size), page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        printf("Could not change the process memory permission at %p\n", ALIGN_ADDR(ptrPltgot, page_size));
    }
    if (mprotect(ALIGN_ADDR(ptrPlt, page_size), page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        printf("Could not change the process memory permission at %p\n", ALIGN_ADDR(ptrPlt, page_size));
    }
    if (mprotect(ALIGN_ADDR(ptrPltSec, page_size), page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        printf("Could not change the process memory permission at %p\n", ALIGN_ADDR(ptrPltSec, page_size));
    }
    hookPLTSEC();

}

__attribute__((destructor)) void libDeConstructor() {
    //Save Statistics
//    printf("DeConstructor\n");

//    free(realAddrResolved);
//    free(hookedNames);
//    free(hookedAddrs);
//    free(hookedIds);
}

int main(int argc, char **argv) {
//    time(NULL);
//    time(NULL);
//    time(NULL);
//    structparm s;
//    int e, f, g, h, i, j, k;
//    long double ld;
//    double m, n;
//    __m256 y;
//    __m512 z;
//    funcEverything(e, f, s, g, h, ld, m, y, z, n, i, j, k);
//
//
//    funcE(2, 3, 4);
//    printf("Main Function Start %d\n", 1);
//    printf("Main Function End %d\n", 2);
//    printf("Main Function End %d\n", 3);
    printf("Main Function End %d\n", 4);
    malloc(1);
    malloc(2);
    malloc(3);
    malloc(4);

//    printf("Test Here %d\n",1);
//    printf("Test Here %d\n",2);
//    printf("Test Here %d\n",3);
//    time(NULL);
//    time(NULL);
//    time(NULL);
//    funcA();
//    funcA();
//    funcA();
//    funcC(1, 2);
//    funcB(2);

    return 0;
}
