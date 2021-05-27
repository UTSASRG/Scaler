#include <cstdio>
#include <cstdlib>


void asmCallPrintf() {
    const char *test = "test\n";
    long dummyreg; /* dummyreg used to allow GCC to pick available register */

    //https://stackoverflow.com/questions/37502841/calling-printf-in-extended-inline-asm/37503773#37503773
    __asm__ __volatile__ (
    "add $-128, %%rsp\n\t"   /* Skip the current redzone */
    "mov %%rsp, %[temp]\n\t" /* Copy RSP to available register */
    "and $-16, %%rsp\n\t"    /* Align stack to 16-byte boundary */
    "mov %[test], %%rdi\n\t" /* RDI is address of string */
    "xor %%eax, %%eax\n\t"   /* Variadic function set AL. This case 0 */
    "call printf\n\t"
    "mov %[test], %%rdi\n\t" /* RDI is address of string again */
    "xor %%eax, %%eax\n\t"   /* Variadic function set AL. This case 0 */
    "call printf\n\t"
    "mov %[temp], %%rsp\n\t" /* Restore RSP */
    "sub $-128, %%rsp\n\t"   /* Add 128 to RSP to restore to orig */
    :  [temp]"=&r"(dummyreg) /* Allow GCC to pick available output register. Modified
                                    before all inputs consumed so use & for early clobber*/
    :  [test]"r"(test),      /* Choose available register as input operand */
            "m"(test)             /* Dummy constraint to make sure test array
                                    is fully realized in memory before inline
                                    assembly is executed */
    : "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
            "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
            "xmm8", "xmm9", "xmm10", "xmm11", "xmm12", "xmm13", "xmm14", "xmm15",
            "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm6",
            "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)"
    );
}

const static char *test = "This is an funcAASM call\n";

extern "C" void asmCallPrintf1() {
    __asm__ __volatile__ (
    "mov %[test], %%rdi\n\t"
    "call printf\n\t"
    :
    :  [test]"r"(test),
            "m"(test)
    :
    );
}

extern "C" void funcA() {
    printf("Hello, this is FuncA %d\n", 1);
}


extern "C" void funcB(int b) {
    printf("Hello, this is FuncB %d\n", b);
}

void asmCallFuncA() {
    __asm__ __volatile__ (
    "call funcA\n\t");
}

void asmCallFuncBWrong() {
    __asm__ __volatile__ (
    "mov $1,%edi\n\t"
    "call funcB\n\t"
    );
}

thread_local int i = 0;

void threadLocalTest() {
    i = 1;
}

static int myarray[] = {1, 2, 3, 4, 5};

void asmGetValueFromArray() {
    for (i = 0; i < 5; ++i) {
        myarray[i];
    }
}

void asmGoTo() {
    int i = 0;
    label1:
    i += 1;
    if (i < 5)
        goto label1;
    printf("");
}

void A() {
    printf("funcAAAAAAAAAAAAAAAAAAAAAAA\n");
}

void B() {
    printf("funcBBBBBBBBBBBBBBBBBBBBBBB\n");
}

extern "C" {
__attribute__((naked)) void callA() {
    __asm__ __volatile__ (
    "lea 0x0(%rip),%rax\n\t"
    "sub -0x1(%rax),%rax\n\t"
    "JMP %rax\n\t"
    );
}
}

void asmCallA() {
    A();
}


int main() {
//    asmCallPrintf();
//    asmCallPrintf1();
//    asmCallFuncA();
//    asmCallFuncBWrong();
//    asmGoTo();
    A();

    return 0;
}
