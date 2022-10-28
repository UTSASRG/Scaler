#include <cstdio>
#include <FuncWithDiffParms.h>
#include <CallFunctionCall.h>
#include <thread>
#include <cassert>

using namespace std;


int main() {
    printf("innerTime, outerTime, cache\n");
    for (int i = 0; i < 10240; ++i) {
        int cache = 0;
        int64_t time1 = getunixtimestampms();
        int64_t innerDuration = funcATimed(cache);
        int64_t outerDuration = getunixtimestampms() - time1;

        printf("%ld,%ld,%d\n", innerDuration, outerDuration, cache);
    }
    return 0;
}