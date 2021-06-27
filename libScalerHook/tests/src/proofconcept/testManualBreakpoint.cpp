
#include <sys/mman.h>
#include <cstdio>

int main() {
    //calling break point
    static unsigned char c[] = {0xcc, 0xc3, 0x12, 0x34, 0x45};
    static void (*pfunc)() =(void (*)()) c;
    static int i = mprotect(reinterpret_cast<void *>((unsigned long int) c & 0xfffffffffffff000), sizeof(c), PROT_EXEC | PROT_READ | PROT_WRITE);
    fprintf(stderr, "Call pfunc"); //put some printf , so that this function is not inlined
    pfunc();
    fprintf(stderr, "Exit pFunc"); //put some printf , so that this function is not inlined
    return 0;
}