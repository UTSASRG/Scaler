#include <CallFunctionCall.h>
#include <FuncWithDiffParms.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    funcA();
    fork();
    callFuncA();
}