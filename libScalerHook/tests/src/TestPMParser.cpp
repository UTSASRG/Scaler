#include <iostream>

#include <util/tool/ProcInfoParser.h>
#include <gtest/gtest.h>
#include <iostream>
#include <util/tool/FileTool.h>

using namespace std;
using namespace scaler;
//TEST(PMParser, openPMMap) {
//    PmParser_Linux pmp;
//}
//
//
//TEST(PMParser, parsePMMap) {
//
//    //Use PMParser to extract /proc/self/map
//    PmParser_Linux pmp;
//
//    //Index entries by addr
//
//    std::map<void *, PMEntry_Linux> entryByAddrMap;
//    auto begIter = pmp.procMap.begin();
//    while (begIter != pmp.procMap.end()) {
//        auto &segVec = (*begIter).second;
//        for (int i = 0; i < segVec.size(); ++i) {
//            entryByAddrMap[segVec[i].addrStart] = segVec[i];
//        }
//        begIter++;
//    }
//
//    std::stringstream ss;
//    std::ifstream f("/proc/self/maps");
//    if (f.is_open())
//        ss << f.rdbuf();
//
//    std::string line;
//    while (std::getline(ss, line)) {
//        printf("Line: %s\n", line.c_str());
//        //Extract starting address
//        int begAddrEnd = line.find(' ');
//        //Find corresponding entry
//        void *curStartAddr = nullptr;
//        void *curEndAddr = nullptr;
//        std::string startAddr = line.substr(0, begAddrEnd);
//        sscanf(startAddr.c_str(), "%p-%p", &curStartAddr, &curEndAddr);
//
//        auto &curSegEntry = entryByAddrMap[curStartAddr];
//
//        //Split output by space
//        vector<size_t> splitIndexes = findStrSplit(line, ' ');
//        //May miss path but other fields should be consistent
//        ASSERT_TRUE(splitIndexes.size() >= 5 * 2);
//        //check if path is the same
//        if (splitIndexes.size() == 12) {
//            //Check if pathname is the same
//            EXPECT_EQ(curSegEntry.pathName, line.substr(splitIndexes[10], splitIndexes[11] - splitIndexes[10]));
//        } else if (splitIndexes.size() != 10) {
//            //# of entires must be 10 or 12.
//            ASSERT_TRUE(false);
//        }
//
//        if (curSegEntry.pathName[0] == '[') {
//            //skip, pathname starting with [ are subject to change
//            continue;
//        }
//
//
//        //Check if address is the same
//        EXPECT_EQ(curStartAddr, curSegEntry.addrStart);
//        EXPECT_EQ(curEndAddr, curSegEntry.addrEnd);
//
//
//        //Check if Permission is the same
//        std::stringstream ss1;
//        ss1.str("");
//        ss1 << (curSegEntry.isR ? 'r' : '-');
//        ss1 << (curSegEntry.isW ? 'w' : '-');
//        ss1 << (curSegEntry.isE ? 'x' : '-');
//        ss1 << (curSegEntry.isP ? 'p' : '-');
//        EXPECT_EQ(ss1.str(), line.substr(splitIndexes[2], splitIndexes[3] - splitIndexes[2]));
//
//
//        //Check if offset is the same
//        char str[10];
//        sprintf(str, "%08x", curSegEntry.offset);
//        EXPECT_EQ(std::string(str), line.substr(splitIndexes[4], splitIndexes[5] - splitIndexes[4]));
//
//
//        //Check if device id is the same
//        EXPECT_EQ(curSegEntry.dev, line.substr(splitIndexes[6], splitIndexes[7] - splitIndexes[6]));
//
//        //Check if inode is the same
//        char str1[256];
//        sprintf(str1, "%d", curSegEntry.inode);
//        EXPECT_EQ(std::string(str1), line.substr(splitIndexes[8], splitIndexes[9] - splitIndexes[8]));
//
//
//    }
//
//}


TEST(PMParser, findExecNameByAddr) {
    //Get current executable file name
    PmParserC_Linux parser;
    PmParserC_Linux parserC;

    void *funcPtr = (void *) printf;
    size_t fileId = parser.findExecNameByAddr(funcPtr);
    auto execName = parser.idFileMap[fileId];
    EXPECT_TRUE(execName.find("libc") != std::string::npos);

    //Try C
    fileId = parserC.findExecNameByAddr(funcPtr);
    execName = parserC.idFileMap[fileId];
    EXPECT_TRUE(execName.find("libc") != std::string::npos);

}