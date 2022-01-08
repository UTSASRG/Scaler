#include <plthook.h>
#include <cstdio>

void test_plthook_enum(plthook_t *plthook) {
    unsigned int pos = 0;
    const char *name;
    void **addr;
    int i;

    while (plthook_enum(plthook, &pos, &name, &addr) == 0) {
        printf("   %s\n", name);
    }

}


//int main() {
//    plthook_t *myPltHook;
//    //Find plthook
//    plthook_open(&myPltHook, NULL);
//    test_plthook_enum(myPltHook);
//}