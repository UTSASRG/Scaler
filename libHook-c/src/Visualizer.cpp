

#include <visualizer/ReportStruct.h>
#include <util/tool/Logging.h>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

vector<string> fileNameMap;
vector<RecSymInfo> libRecordArr;

void readFileName(const char *dataFolder) {

    stringstream ss;
    ss << dataFolder << "/fileName.txt";

    FILE *fd = fopen(ss.str().c_str(), "r");
    if (!fd) { fatalErrorS("Cannot read fileName.txt because: %s", strerror(errno)); }
    //Skip one line
    fscanf(fd, "%*[^\n]");

    char fileIdStr[256];
    char fileName[PATH_MAX];
    while (fscanf(fd, "%s,%s", fileIdStr, fileName) == 2) {
        fileNameMap.emplace_back(std::string(fileName));
    }
}

void readSymbolId(const char *dataFolder) {

    stringstream ss;
    ss << dataFolder << "/symbolInfo.txt";

    FILE *fd = fopen(ss.str().c_str(), "r");
    if (!fd) { fatalErrorS("Cannot read fileName.txt because: %s", strerror(errno)); }
    //Skip one line
    fscanf(fd, "%*[^\n]");

    RecSymInfo rec;
    char funcName[256];
    while (fscanf(fd, "%s,%zd,%zd", funcName, &rec.fileId, &rec.symIdInFile) == 3) {
        rec.funcName = funcName;
        libRecordArr.emplace_back(rec);
    }
}

AppRecord record;

int main(int argc, char *argv[]) {
    INFO_LOGS("Visualizer Ver %s", CMAKE_SCALERRUN_VERSION);
    const char *dataFolder = argv[1];

    readFileName(dataFolder);
    readSymbolId(dataFolder);

    //Fill libRec
    for (int i = 0; i < fileNameMap.size(); ++i) {
        LibRecord libRec;
        libRec.libPath = fileNameMap[i];
        record.libRecordArr.emplace_back(libRec);
    }




}
