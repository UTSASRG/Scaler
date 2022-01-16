


#include <cstdlib>
#include <sstream>


int main(int argc, char **argv) {
    std::stringstream ss;
    std::string scalerBin(argv[1]);
    ss<<"LD_PRELOAD="<<scalerBin<<" ";
    for (int i = 2; i < argc; ++i) {
        ss << argv[i] << " ";
    }
    printf("%s\n",ss.str().c_str());
    return system(ss.str().c_str());
return 0;
}