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

TEST(PMParser, ParseAfterDLOpen) {
    std::stringstream ss;
    ss << PROJECT_SOURCE_DIR << "/src/unittests/testinputs/PmParser/test1-TestDL-step1-beforedlopen.txt";
    PmParser parser("/tmp", ss.str());

    Array<int> newlyLoadedFileId(5);
    Array<int> newlyLoadedPmEntry(5);
    parser.parsePMMap(0);
    ASSERT_EQ(parser.fileEntryArray.getSize(), 9);
    ASSERT_EQ(parser.pmEntryArray.getSize(), 21);


    uint8_t *pmEntryStartingAddrs[] = {(uint8_t *) 0x555555554000, (uint8_t *) 0x555555754000,
                                       (uint8_t *) 0x555555755000, (uint8_t *) 0x7ffff77de000,
                                       (uint8_t *) 0x7ffff79c5000, (uint8_t *) 0x7ffff7bc5000,
                                       (uint8_t *) 0x7ffff7bc9000, (uint8_t *) 0x7ffff7bcb000,
                                       (uint8_t *) 0x7ffff7bcf000, (uint8_t *) 0x7ffff7bd2000,
                                       (uint8_t *) 0x7ffff7dd1000, (uint8_t *) 0x7ffff7dd2000,
                                       (uint8_t *) 0x7ffff7dd3000, (uint8_t *) 0x7ffff7fd6000,
                                       (uint8_t *) 0x7ffff7ff7000, (uint8_t *) 0x7ffff7ffa000,
                                       (uint8_t *) 0x7ffff7ffc000, (uint8_t *) 0x7ffff7ffd000,
                                       (uint8_t *) 0x7ffff7ffe000, (uint8_t *) 0x7ffffffde000,
                                       (uint8_t *) 0xffffffffff600000};
    uint8_t *pmEntryEndAddrs[] = {(uint8_t *) 0x555555555000, (uint8_t *) 0x555555755000, (uint8_t *) 0x555555756000,
                                  (uint8_t *) 0x7ffff79c5000, (uint8_t *) 0x7ffff7bc5000, (uint8_t *) 0x7ffff7bc9000,
                                  (uint8_t *) 0x7ffff7bcb000, (uint8_t *) 0x7ffff7bcf000, (uint8_t *) 0x7ffff7bd2000,
                                  (uint8_t *) 0x7ffff7dd1000, (uint8_t *) 0x7ffff7dd2000, (uint8_t *) 0x7ffff7dd3000,
                                  (uint8_t *) 0x7ffff7dfc000, (uint8_t *) 0x7ffff7fdb000, (uint8_t *) 0x7ffff7ffa000,
                                  (uint8_t *) 0x7ffff7ffc000, (uint8_t *) 0x7ffff7ffd000, (uint8_t *) 0x7ffff7ffe000,
                                  (uint8_t *) 0x7ffff7fff000, (uint8_t *) 0x7ffffffff000, (uint8_t *) 0xffffffffff601000
    };
    unsigned char permBits[] = {0b1011, 0b1001, 0b1101, 0b1011, 0b0001, 0b1001, 0b1101, 0b1101, 0b1011, 0b0001, 0b1001,
                                0b1101, 0b1011, 0b1101, 0b1001, 0b1011, 0b1001, 0b1101, 0b1101, 0b1101, 0b1011};
    int fileIds[] = {0, 0, 0, 1, 1, 1, 1, 2, 3, 3, 3, 3, 4, 2, 5, 6, 4, 4, 2, 7, 8};
    int pmEntryNumbers[] = {3, 4, 3, 4, 3, 1, 1, 1, 1};
    std::string strList[] = {
            "/home/steven/Projects/Scaler/cmake-build-debug/libHook-c/tests/src/proofconcept/TestDL",
            "/lib/x86_64-linux-gnu/libc-2.27.so",
            "",
            "/lib/x86_64-linux-gnu/libdl-2.27.so",
            "/lib/x86_64-linux-gnu/ld-2.27.so",
            "[vvar]",
            "[vdso]",
            "[stack]",
            "[vsyscall]"
    };

    for (int i = 0; i < parser.fileEntryArray.getSize(); ++i) {
        FileEntry &curFileEntry = parser.fileEntryArray[i];
        ASSERT_EQ(std::string(&parser.stringTable.get(curFileEntry.pathNameStartIndex)), strList[i]);
        ASSERT_EQ(curFileEntry.pmEntryNumbers, pmEntryNumbers[i]);
        ASSERT_EQ(curFileEntry.loadingId, 0);
    }

    for (int i = 0; i < parser.pmEntryArray.getSize(); ++i) {
        PMEntry &curPmEntry = parser.pmEntryArray[i];
        ASSERT_EQ(curPmEntry.addrStart, pmEntryStartingAddrs[i]);
        ASSERT_EQ(curPmEntry.addrEnd, pmEntryEndAddrs[i]);
        ASSERT_EQ(curPmEntry.permBits, permBits[i]);
        ASSERT_EQ(curPmEntry.fileId, fileIds[i]);
        ASSERT_EQ(curPmEntry.loadingId, 0);
    }

    ssize_t newFileSize = 0;
    for (int i = 0; i < parser.fileEntryArray.getSize(); ++i) {
        ASSERT_EQ(parser.fileEntryArray[i].pmEntryNumbers, pmEntryNumbers[i]);
        if (parser.fileEntryArray[i].creationLoadingId == 0) {
            newFileSize += 1;
        }
    }
    ASSERT_EQ(newFileSize, 9);

    newlyLoadedFileId.clear();
    newlyLoadedPmEntry.clear();
    ss.str("");
    ss << PROJECT_SOURCE_DIR << "/src/unittests/testinputs/PmParser/test1-TestDL-step2-afterdlopen.txt";
    parser.customProcFileName = ss.str();
    //Test on the reuslt after dlopen
    parser.parsePMMap(1);

    uint8_t *pmEntryStartingAddrs1[] = {(uint8_t *) 0x555555554000, (uint8_t *) 0x555555754000,
                                        (uint8_t *) 0x555555755000, (uint8_t *) 0x555555756000,
                                        (uint8_t *) 0x7ffff7448000, (uint8_t *) 0x7ffff75dc000,
                                        (uint8_t *) 0x7ffff77dc000, (uint8_t *) 0x7ffff77dd000,
                                        (uint8_t *) 0x7ffff77de000, (uint8_t *) 0x7ffff79c5000,
                                        (uint8_t *) 0x7ffff7bc5000, (uint8_t *) 0x7ffff7bc9000,
                                        (uint8_t *) 0x7ffff7bcb000, (uint8_t *) 0x7ffff7bcf000,
                                        (uint8_t *) 0x7ffff7bd2000, (uint8_t *) 0x7ffff7dd1000,
                                        (uint8_t *) 0x7ffff7dd2000, (uint8_t *) 0x7ffff7dd3000,
                                        (uint8_t *) 0x7ffff7fd6000, (uint8_t *) 0x7ffff7ff7000,
                                        (uint8_t *) 0x7ffff7ffa000, (uint8_t *) 0x7ffff7ffc000,
                                        (uint8_t *) 0x7ffff7ffd000, (uint8_t *) 0x7ffff7ffe000,
                                        (uint8_t *) 0x7ffffffde000, (uint8_t *) 0xffffffffff600000
    };
    uint8_t *pmEntryEndAddrs1[] = {(uint8_t *) 0x555555555000, (uint8_t *) 0x555555755000, (uint8_t *) 0x555555756000,
                                   (uint8_t *) 0x555555777000, (uint8_t *) 0x7ffff75dc000, (uint8_t *) 0x7ffff77dc000,
                                   (uint8_t *) 0x7ffff77dd000, (uint8_t *) 0x7ffff77de000, (uint8_t *) 0x7ffff79c5000,
                                   (uint8_t *) 0x7ffff7bc5000, (uint8_t *) 0x7ffff7bc9000, (uint8_t *) 0x7ffff7bcb000,
                                   (uint8_t *) 0x7ffff7bcf000, (uint8_t *) 0x7ffff7bd2000, (uint8_t *) 0x7ffff7dd1000,
                                   (uint8_t *) 0x7ffff7dd2000, (uint8_t *) 0x7ffff7dd3000, (uint8_t *) 0x7ffff7dfc000,
                                   (uint8_t *) 0x7ffff7fdb000, (uint8_t *) 0x7ffff7ffa000, (uint8_t *) 0x7ffff7ffc000,
                                   (uint8_t *) 0x7ffff7ffd000, (uint8_t *) 0x7ffff7ffe000, (uint8_t *) 0x7ffff7fff000,
                                   (uint8_t *) 0x7ffffffff000, (uint8_t *) 0xffffffffff601000};

    unsigned char permBits1[] = {0b1011, 0b1001, 0b1101, 0b1101, 0b1011, 0b0001, 0b1001, 0b1101, 0b1011, 0b0001, 0b1001,
                                 0b1101, 0b1101, 0b1011, 0b0001, 0b1001, 0b1101, 0b1011, 0b1101, 0b1001, 0b1011, 0b1001,
                                 0b1101, 0b1101, 0b1101, 0b1011};
    int fileIds1[] = {0, 0, 0, 9, 10, 10, 10, 10, 1, 1, 1, 1, 2, 3, 3, 3, 3, 4, 2, 5, 6, 4, 4, 2, 7, 8};
    int pmEntryNumbers1[] = {3, 4, 3, 4, 3, 1, 1, 1, 1, 1, 4};
    std::string strList1[] = {
            "/home/steven/Projects/Scaler/cmake-build-debug/libHook-c/tests/src/proofconcept/TestDL",
            "/lib/x86_64-linux-gnu/libc-2.27.so",
            "",
            "/lib/x86_64-linux-gnu/libdl-2.27.so",
            "/lib/x86_64-linux-gnu/ld-2.27.so",
            "[vvar]",
            "[vdso]",
            "[stack]",
            "[vsyscall]",
            "[heap]",
            "/home/steven/Projects/Scaler/cmake-build-release/libHook-c/tests/libTestlib-FuncCall.so"
    };

    for (int i = 0; i < parser.fileEntryArray.getSize(); ++i) {
        FileEntry &curFileEntry = parser.fileEntryArray[i];
        ASSERT_EQ(std::string(&parser.stringTable.get(curFileEntry.pathNameStartIndex)), strList1[i]);
        ASSERT_EQ(curFileEntry.pmEntryNumbers, pmEntryNumbers1[i]);
        ASSERT_EQ(curFileEntry.loadingId, 1);
    }

    for (int i = 0; i < parser.pmEntryArray.getSize(); ++i) {
        PMEntry &curPmEntry = parser.pmEntryArray[i];
        ASSERT_EQ(curPmEntry.addrStart, pmEntryStartingAddrs1[i]);
        ASSERT_EQ(curPmEntry.addrEnd, pmEntryEndAddrs1[i]);
        ASSERT_EQ(curPmEntry.permBits, permBits1[i]);
        ASSERT_EQ(curPmEntry.fileId, fileIds1[i]);
        ASSERT_EQ(curPmEntry.loadingId, 1);
    }

    newFileSize = 0;
    Array<FileID> curFileIdArr;
    parser.getNewFileEntryIds(1, curFileIdArr,false);
    ASSERT_EQ(curFileIdArr.getSize(), 2); //Heap and the new testDL
    curFileIdArr.clear();
    parser.getNewFileEntryIds(1, curFileIdArr,true);
    ASSERT_EQ(curFileIdArr.getSize(), 1); //Heap and the new testDL

}


TEST(PMParser, ParseAfterPmEntryDeletion) {
    std::stringstream ss;
    ss << PROJECT_SOURCE_DIR << "/src/unittests/testinputs/PmParser/test1-TestDL-step1-beforedlopen.txt";
    PmParser parser("/tmp", ss.str());
    parser.parsePMMap(0);

    ss.str("");
    ss << PROJECT_SOURCE_DIR << "/src/unittests/testinputs/PmParser/test1-TestDL-step2-afterdlopen.txt";
    parser.customProcFileName = ss.str();
    //Test on the reuslt after dlopen
    parser.parsePMMap(1);

    ss.str("");
    ss << PROJECT_SOURCE_DIR << "/src/unittests/testinputs/PmParser/test1-TestDL-step3-afterdeletingPmEntry.txt";
    parser.customProcFileName = ss.str();
    //Test on the reuslt after dlopen
    parser.parsePMMap(2);

    int pmEntryNumbers2[] = {3, 3, 3, 4, 2, 1, 0, 1, 1, 1, 4};

    ssize_t validFileSize = 0;
    ssize_t newFileSize = 0;
    for (int i = 0; i < parser.fileEntryArray.getSize(); ++i) {
        ASSERT_EQ(parser.fileEntryArray[i].pmEntryNumbers, pmEntryNumbers2[i]);
        //INFO_LOGS("%s", parser.stringTable.substr(parser.fileEntryArray[i].pathNameStartIndex,
        //                                          parser.fileEntryArray[i].getPathNameLength()).c_str());
        if (parser.fileEntryArray[i].creationLoadingId == 2) {
            newFileSize += 1;
        }
        if (parser.fileEntryArray[i].loadingId == 2) {
            validFileSize += 1;
        } else {
            ASSERT_EQ(std::string(&parser.stringTable.get(parser.fileEntryArray[i].pathNameStartIndex)), "[vdso]");
        }
    }
    ASSERT_EQ(parser.fileEntryArray.getSize(), 11); //FileArray should contain all the files loaded before
    ASSERT_EQ(validFileSize, 10);//<vdso> should be already deleted, so there is one tiem less
    ASSERT_EQ(parser.pmEntryArray.getSize(), 23); //PmEntryArray should reflect the actual number of pmEntry
    ASSERT_EQ(newFileSize, 0);

}


TEST(PMParser, LibraryReplaced) {
    std::stringstream ss;
    ss << PROJECT_SOURCE_DIR << "/src/unittests/testinputs/PmParser/test1-TestDL-step1-beforedlopen.txt";
    PmParser parser("/tmp", ss.str());
    parser.parsePMMap(0);

    //for(ssize_t i=0;i<parser.fileEntryArray.getSize();++i){
    //    DBG_LOGS("FileId:%zd PathName:%s",i,parser.getStrUnsafe(parser.getFileEntry(i).pathNameStartIndex));
    //}


    ss.str("");
    ss << PROJECT_SOURCE_DIR << "/src/unittests/testinputs/PmParser/test1-TestDL-step2-afterdlopen.txt";
    parser.customProcFileName = ss.str();
    //Test on the reuslt after dlopen
    parser.parsePMMap(1);

//    for(ssize_t i=0;i<parser.fileEntryArray.getSize();++i){
//        DBG_LOGS("FileId:%zd PathName:%s",i,parser.getStrUnsafe(parser.getFileEntry(i).pathNameStartIndex));
//    }


    ss.str("");
    ss << PROJECT_SOURCE_DIR << "/src/unittests/testinputs/PmParser/test1-TestDL-step3-afterdeletingPmEntry.txt";
    parser.customProcFileName = ss.str();
    //Test on the reuslt after dlopen
    parser.parsePMMap(2);

//    for(ssize_t i=0;i<parser.fileEntryArray.getSize();++i){
//        DBG_LOGS("FileId:%zd PathName:%s",i,parser.getStrUnsafe(parser.getFileEntry(i).pathNameStartIndex));
//    }

    ss.str("");
    ss << PROJECT_SOURCE_DIR << "/src/unittests/testinputs/PmParser/test1-TestDL-step4-LibraryReplaced.txt";
    parser.customProcFileName = ss.str();
    //Test on the reuslt after dlopen
    parser.parsePMMap(3);

//    for(ssize_t i=0;i<parser.fileEntryArray.getSize();++i){
//        DBG_LOGS("FileId:%zd PathName:%s",i,parser.getStrUnsafe(parser.getFileEntry(i).pathNameStartIndex));
//    }


    int pmEntryNumbers[] = {3, 1, 3, 4, 2, 1, 0, 1, 1, 1, 4, 2};


    Array<FileID> curFileIdArr;
    parser.getNewFileEntryIds(3, curFileIdArr);

    ASSERT_EQ(parser.fileEntryArray.getSize(), 12); //FileArray should contain all the files loaded before
    ASSERT_EQ(curFileIdArr.getSize(), 1);//<vdso> should be already deleted, so there is one tiem less
    ASSERT_EQ(parser.pmEntryArray.getSize(), 23); //PmEntryArray should reflect the actual number of pmEntry

}


TEST(PMParser, findExecNameByAddr) {
    //Get current executable file name
//    PmParser parser;
//
//    void *funcPtr = (void *) printf;
//    size_t fileId = parser.findFileIdByAddr(funcPtr);
//    auto execName = parser.idFileMap[fileId];
//    EXPECT_TRUE(execName.find("libc") != std::string::npos);
//
//    //Try C
//    fileId = parserC.findFileIdByAddr(funcPtr);
//    execName = parserC.idFileMap[fileId];
//    EXPECT_TRUE(execName.find("libc") != std::string::npos);

}