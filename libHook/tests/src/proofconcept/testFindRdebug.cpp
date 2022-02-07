#include <plthook.h>
#include <elf.h>
#include <link.h>
#include <libtest.h>

int main() {
    r_debug *_myRdebug = nullptr;
    const ElfW(Dyn) *dyn = _DYNAMIC;
    for (dyn = _DYNAMIC; dyn->d_tag != DT_NULL; ++dyn)
        if (dyn->d_tag == DT_DEBUG)
            _myRdebug = (struct r_debug *) dyn->d_un.d_ptr;
    printf("My Rdbg is: %p\n", _myRdebug);
    printf("Library Rdbg is: %p\n", findRdbg());
    printf("My _DYNAMIC is: %p\n", _DYNAMIC);
    printf("Library _DYNAMIC is: %p\n", findDYNAMIC());


}
