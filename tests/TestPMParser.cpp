#include <iostream>

#include <util/tool/PMParser.h>
#include <gtest/gtest.h>
#include <iostream>
#include <util/tool/FileTool.h>

using namespace std;
using namespace scaler;
TEST(PMParser, openPMMap) {
    PMParser pmp;
}


TEST(PMParser, parsePMMap) {

    //Use PMParser to extract /proc/self/map
    PMParser pmp;
    pmp.parsePMMap();

    //Index entries by addr

    std::map<std::string, PMEntry> entryByAddrMap;
    auto begIter = pmp.procMap.begin();
    while (begIter != pmp.procMap.end()) {
        auto &segVec = (*begIter).second;
        for (int i = 0; i < segVec.size(); ++i) {
            auto addr = reinterpret_cast<long long int>(segVec[i].addrStart);
            std::stringstream ss;
            ss << std::noshowbase << std::hex << addr;
            entryByAddrMap[ss.str()] = segVec[i];
        }
        begIter++;
    }

    std::stringstream ss;
    std::ifstream f("/proc/self/maps");
    if (f.is_open())
        ss << f.rdbuf();

    std::string name;
    while (std::getline(ss, name)) {
        printf("%s\n", name.c_str());
        //Extract starting address
        int begAddrEnd = name.find('-');
        //Find corresponding entry
        auto &curSegEntry = entryByAddrMap[name.substr(0, begAddrEnd)];
        //Split output by space
        vector<size_t> splitIndexes = findStrSplit(name, ' ');

        //May miss path but other fields should be consistent
        ASSERT_TRUE(splitIndexes.size() >= 5 * 2);

        //Check if address is the same
        std::stringstream ss;
        ss << std::hex << std::noshowbase << (long long int) (curSegEntry.addrStart) << "-"
           << (long long int) (curSegEntry.addrEnd);
        EXPECT_EQ(ss.str(), name.substr(splitIndexes[0], splitIndexes[1] - splitIndexes[0]));


        //Check if Permission is the same
        ss.str("");
        ss << (curSegEntry.isR ? 'r' : '-');
        ss << (curSegEntry.isW ? 'w' : '-');
        ss << (curSegEntry.isE ? 'x' : '-');
        ss << (curSegEntry.isP ? 'p' : '-');
        EXPECT_EQ(ss.str(), name.substr(splitIndexes[2], splitIndexes[3] - splitIndexes[2]));


        //Check if offset is the same
        char str[10];
        sprintf(str, "%08x", curSegEntry.offset);
        EXPECT_EQ(std::string(str), name.substr(splitIndexes[4], splitIndexes[5] - splitIndexes[4]));


        //Check if device id is the same
        EXPECT_EQ(curSegEntry.dev, name.substr(splitIndexes[6], splitIndexes[7] - splitIndexes[6]));

        //Check if inode is the same
        char str1[256];
        sprintf(str1, "%d", curSegEntry.inode);
        EXPECT_EQ(std::string(str1), name.substr(splitIndexes[8], splitIndexes[9] - splitIndexes[8]));

        if (splitIndexes.size() == 12) {
            //Check if pathname is the same
            EXPECT_EQ(curSegEntry.pathName, name.substr(splitIndexes[10], splitIndexes[11] - splitIndexes[10]));
        } else {
            //If there's more entry, there's might be a mistake.
            ASSERT_TRUE(false);
        }
    }

}


//int main() {
//    return 0;
//}
