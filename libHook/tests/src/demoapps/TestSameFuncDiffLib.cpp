
#include <FuncWithDiffParms.h>

int main() {

    void* p1 = malloc(1024);
    void* p2 = callMalloc(1024);

    return p1 > p2 ? 0 : 1;
}