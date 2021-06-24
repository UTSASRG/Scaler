#include <plthook.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "libtest.h"
#include <dlfcn.h>
#include <functional>

typedef struct {
    const char *name;
    int enumerated;
} enum_test_data_t;

static enum_test_data_t funcs_called_by_main[] = {
        {"strtod_cdecl", 0},
        {NULL,},
};

#define STRTOD_STR_SIZE 30

typedef struct {
    char str[STRTOD_STR_SIZE];
    double result;
} hooked_val_t;

/* value captured by hook from executable to libtest. */
static hooked_val_t val_exe2lib;
/* value captured by hook from libtest to libc. */
static hooked_val_t val_lib2libc;

static void reset_result(void) {
    val_exe2lib.str[0] = '\0';
    val_exe2lib.result = 0.0;
    val_lib2libc.str[0] = '\0';
    val_lib2libc.result = 0.0;
}

static void set_result(hooked_val_t *hv, const char *str, double result) {
    strncpy(hv->str, str, sizeof(hv->str));
    hv->result = result;
}


extern "C" {

const static char *test = "This is an hookFuncAsm call\n";

void hookFuncPrint() {
    printf("Hooked");
}

void hookFuncAsm() {
    __asm__ __volatile__ (
    "mov %[test], %%rdi\n\t"
    "call printf\n\t"
    :
    :  [test]"r"(test),
            "m"(test)
    :
    );
}

void printSth() {
    printf("Print Some Thing");
}

void hookFuncJump() {

    __asm__ __volatile__ (
    "call printSth\n\t"
    );

}
}


static void test_plthook_enum(plthook_t *plthook) {
    unsigned int pos = 0;
    const char *name;
    void **addr;
    int i;

    while (plthook_enum(plthook, &pos, &name, &addr) == 0) {
        printf("   %s\n", name);
    }

}


int main(int argc, char **argv) {
    plthook_t *myPltHook;
    //Find plthook
    plthook_open(&myPltHook, NULL);
    //Check plt hook entry size
    myPltHook->rela_plt_cnt;


    test_plthook_enum(myPltHook);
//    printf("HookAddr: %#010x\n", hookFuncJump);
    void **oldFunc;
    plthook_replace(myPltHook, "funcA", (void *) hookFuncJump, (void **) &oldFunc);
    funcA();
    plthook_close(myPltHook);

    return 0;
}
