#include <util/hook/ExtFuncCallHook.hh>
#include <util/tool/PMParser.h>
#include <cstdint>
#include <util/tool/ElfParser.h>
#include <util/tool/FileTool.h>
#include <exceptions/ScalerException.h>

namespace scaler {
#define PUSHXMM(ArgumentName) \
"subq $16,%rsp\n\t" \
"movdqu  %xmm"#ArgumentName" ,(%rsp)\n\t"

#define POPXMM(ArgumentName) \
"movdqu  (%rsp),%xmm"#ArgumentName"\n\t"\
"addq $16,%rsp\n\t"


#define PUSH64bits(ArgumentName) \
"subq $8,%rsp\n\t" \
"movdqu  %"#ArgumentName",(%rsp)\n\t"

#define POP64bits(ArgumentName) \
"movdqu  (%rsp),%"#ArgumentName"\n\t"\
"addq $16,%rsp\n\t"

#define ALIGN_ADDR(addr, page_size) (void *) ((size_t) (addr) / page_size * page_size)

    void ExtFuncCallHook::locSectionInMem() {

        //Get segment info from /proc/self/maps
        PMParser pmParser;
        pmParser.parsePMMap();

        //Iterate through libraries
        for (auto iter = pmParser.procMap.begin(); iter != pmParser.procMap.end(); ++iter) {
            //Open corresponding ELF file
            ELFParser elfParser(iter->first);

            try {
                elfParser.parse();

                auto &curFile = fileSecMap[iter->first];

                auto &curPLT = curFile[".plt"];
                curPLT.startAddr = searchSecLoadingAddr(".plt", elfParser, iter->second);

                auto &curPLTSec = curFile[".plt.sec"];
                curPLTSec.startAddr = searchSecLoadingAddr(".plt.sec", elfParser, iter->second);

                auto &curPLTGot = curFile[".plt.got"];
                curPLTGot.startAddr = searchSecLoadingAddr(".plt.got", elfParser, iter->second);

                //Make sure every section is found
                for (auto iter1 = curFile.begin();
                     iter1 != curFile.end(); ++iter1) {
                    if (iter1->second.startAddr == nullptr) {
                        std::stringstream ss;
                        ss << "Section " << iter1->first << " not found in " << iter->first;
                        throwScalerException(ss.str().c_str());
                    }
                }

                //We already have the starting address, let's calculate the end address
                curPLT.endAddr =
                        (char *) curPLT.startAddr + elfParser.getSecLength(".plt");
                curPLTSec.endAddr =
                        (char *) curPLTSec.startAddr + elfParser.getSecLength(".plt.sec");
                curPLTGot.endAddr =
                        (char *) curPLTGot.startAddr + elfParser.getSecLength(".plt.got");

            } catch (const ScalerException &e) {
                std::stringstream ss;
                ss << "Hook Failed for \"" << elfParser.elfPath << "\" because " << e.info;
                fprintf(stderr, "%s\n", ss.str().c_str());
            }

        }


    }

    void *ExtFuncCallHook::searchSecLoadingAddr(const std::string &secName, ELFParser &elfParser,
                                                const std::vector<PMEntry> &segments) {

        //Read the section specified by secName from ELF file.
        void *secPtr = elfParser.getSecPtr(secName);
        if (!secPtr) {
            std::stringstream ss;
            ss << "Cannot find section " << secName << " in " << elfParser.elfPath;
            throwScalerException(ss.str().c_str());
        }

        void *rltAddr = nullptr;

        //Match the first 20 bytes
        for (int i = 0; i < segments.size(); ++i) {
            if (segments[i].isE) {
                //Only check executable secitons
                rltAddr = binarySearch(segments[i].addrStart, segments[i].length, secPtr, 20);
                if (rltAddr)
                    break;
            }
        }
        return rltAddr;
    }

    void ExtFuncCallHook::install() {
        //Step1: Locating call PLT
        locSectionInMem();
        //Step2: Replace PLT
        loadPltNames();


    }

    void ExtFuncCallHook::loadPltNames() {


    }


}

