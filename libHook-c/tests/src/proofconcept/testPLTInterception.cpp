
#include <cstdio>
#include <cstring>
#include <limits.h>
#include <sys/mman.h>
#include <unistd.h>


extern char __startplt, __startpltgot, __startpltsec;
extern char *ptrPlt = nullptr, *ptrPltgot = nullptr, *ptrPltsec = nullptr;

#define ALIGN_ADDR(addr) (void *) ((size_t) (addr) / page_size * page_size)

int printFile(const char *fileName) {
    FILE *fptr;
    char c;
    // Open file
    fptr = fopen(fileName, "r");
    if (fptr == NULL) {
        printf("Cannot open file \n");
        return -1;
    }
    // Read contents from file
    c = fgetc(fptr);
    while (c != EOF) {
        printf("%c", c);
        c = fgetc(fptr);
    }
    fclose(fptr);
    return 0;
}


int main(int argc, char **argv) {
    ptrPlt = &__startplt;
    ptrPltgot = &__startpltgot;
    ptrPltsec = &__startpltsec;

    printf("__startplt: %p\n", ptrPlt);
    printf("__startpltgot: %p\n", ptrPltgot);
    printf("__startpltsec: %p\n", ptrPltsec);

    printFile("/proc/self/maps");

    //Remove protection
    static size_t page_size = sysconf(_SC_PAGESIZE);
    int retCode = mprotect(ALIGN_ADDR(ptrPlt), page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    if (retCode != 0) {
        printf("Could not change the process memory permission at %p\n", ALIGN_ADDR(ptrPlt));
    }
    if (mprotect(ALIGN_ADDR(ptrPltgot), page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        printf("Could not change the process memory permission at %p\n", ALIGN_ADDR(ptrPltgot));
    }
    if (mprotect(ALIGN_ADDR(ptrPltsec), page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        printf("Could not change the process memory permission at %p\n", ALIGN_ADDR(ptrPltsec));
    }

    //Fill with 0
    char magicNumber[] = {0, 0, 0, 0, 0, 0, 0, 0};
    //memcpy(ptrPlt, magicNumber, sizeof(char) * 8);
    //memcpy(ptrPltgot, magicNumber, sizeof(char) * 8);
    memcpy(ptrPltsec, magicNumber, sizeof(char) * 8);



    return 0;
}