#include <plthook.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <link.h>
#include <cstring>
#include <vector>
#include <libtest.h>
#include <string>

//https://stackoverflow.com/questions/10161911/push-xmm-register-to-the-stack

#define PUSHXMM(ArgumentName) \
fprintf(fp, "\"subq $16,%%rsp\\n\\t\"\n", *addr); \
fprintf(fp, "\"movdqu  %%xmm"#ArgumentName" ,(%%rsp)\\n\\t\"\n", *addr);

#define POPXMM(ArgumentName) \
fprintf(fp, "\"movdqu  (%%rsp),%%xmm"#ArgumentName"\\n\\t\"\n", *addr);\
fprintf(fp, "\"addq $16,%%rsp\\n\\t\"\n", *addr);


using namespace std;
vector<int> hookedIds;
vector<string> hookedNames;


void CHookHanlder(int index) {
    printf("CHookHandler %s is called\n", hookedNames[index].c_str());
}

static void writeAndCompileHookHanlder(plthook_t *plthook) {
    FILE *fp = NULL;

    fp = fopen("./testHandler.cpp", "w");
    unsigned int pos = 0;
    const char *name;
    void **addr;
    int i;
    fprintf(fp, "extern \"C\"{\n");
    while (plthook_enum(plthook, &pos, &name, &addr) == 0) {
        int result = strncmp(name, "func", 4);
        if (result == 0) {
            printf("%s Hooked\n", name);
            fprintf(fp, "void  __attribute__((naked)) pltHandler%d(){\n", pos);
            fprintf(fp, "__asm__ __volatile__ (\n", *addr);

            //https://stackoverflow.com/questions/29112919/errors-when-pushing-xmm-registers-onto-stack

            //Save all registers
            fprintf(fp, "//%s\n", name);
            fprintf(fp, "\"push %%rdi\\n\\t\"\n", *addr);
            fprintf(fp, "\"push %%rsi\\n\\t\"\n", *addr);
            fprintf(fp, "\"push %%rdx\\n\\t\"\n", *addr);
            fprintf(fp, "\"push %%rcx\\n\\t\"\n", *addr);
            fprintf(fp, "\"push %%r8\\n\\t\"\n", *addr);
            fprintf(fp, "\"push %%r9\\n\\t\"\n", *addr);
            PUSHXMM(0)
            PUSHXMM(1)
            PUSHXMM(2)
            PUSHXMM(3)
            PUSHXMM(4)
            PUSHXMM(5)
            PUSHXMM(6)
            PUSHXMM(7)

            //call hook function
            fprintf(fp, "\"mov $%d,%%rdi\\n\\t\"\n", hookedIds.size(), *addr);
            fprintf(fp, "\"movq $%p,%%r15\\n\\t\"\n", *CHookHanlder);
            fprintf(fp, "\"call  *%%r15\\n\\t\"\n");


            //Restore all registers
            POPXMM(7)
            POPXMM(6)
            POPXMM(5)
            POPXMM(4)
            POPXMM(3)
            POPXMM(2)
            POPXMM(1)
            POPXMM(0)
            fprintf(fp, "\"pop %%r9\\n\\t\"\n", *addr);
            fprintf(fp, "\"pop %%r8\\n\\t\"\n", *addr);
            fprintf(fp, "\"pop %%rcx\\n\\t\"\n", *addr);
            fprintf(fp, "\"pop %%rdx\\n\\t\"\n", *addr);
            fprintf(fp, "\"pop %%rsi\\n\\t\"\n", *addr);
            fprintf(fp, "\"pop %%rdi\\n\\t\"\n", *addr);




            // Call function Handler before
            fprintf(fp, "\"movq $%p,%%r15\\n\\t\"\n", *addr);
            fprintf(fp, "\"jmp  *%%r15\\n\\t\"\n");


            fprintf(fp, ");\n", *addr);
            fprintf(fp, "}\n", pos);
            hookedIds.push_back(pos);
            hookedNames.push_back(name);
        }
    }

    fprintf(fp, "}\n");
    fclose(fp);
    system("gcc -shared -fPIC ./testHandler.cpp -o ./testHandler.so");
}

int main(int argc, char **argv) {
    funcA();
    funcA();
    funcB(1);
    funcC(1, 2);
    funcD(1, 2, 3);
    funcE(1, 2, 3);

    plthook_t *myPltHook;
    //Find plthook
    //plthook_open(&myPltHook, NULL);

    r_debug *_myRdebug = nullptr;
    const ElfW(Dyn) *dyn = _DYNAMIC;
    for (dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn)
        if (dyn->d_tag == DT_DEBUG)
            _myRdebug = (struct r_debug *) dyn->d_un.d_ptr;
    plthook_open_real(&myPltHook, _myRdebug->r_map);

    //Check plt hook entry size
    writeAndCompileHookHanlder(myPltHook);
    void *handle = dlopen("/home/st/Projects/PLT/plthook/cmake-build-release/testHandler.so", RTLD_NOW);

    for (int i = 0; i < hookedIds.size(); ++i) {
        char output[20];
        char *funcName = "pltHandler%d";
        sprintf(output, funcName, hookedIds[i]);
        void *functionAddr = dlsym(handle, output);
        if (functionAddr == nullptr) {
            char *errstr = dlerror();
            if (errstr != NULL)
                printf("A dynamic linking error occurred: (%s)\n", errstr);
        } else {
            printf("FunctionAddr for %s:%p\n", hookedNames[i].c_str(), functionAddr);
        }
        void *oldFunc = nullptr;
        plthook_replace(myPltHook, hookedNames[i].c_str(), functionAddr, &oldFunc);
    }
    funcA();
    for (int i = 0; i < 100; ++i) {
        funcB(1);
    }
    funcC(1, 2);
    funcD(1, 2, 3);
    funcE(1, 2, 3);
//    printf("%p\n", handle);
    return 0;
}

