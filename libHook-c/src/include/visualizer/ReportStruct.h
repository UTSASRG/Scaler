#ifndef SCALER_REPORTSTRUCT_H
#define SCALER_REPORTSTRUCT_H

#include <string>
#include <vector>
#include <util/hook/HookContext.h>

template<typename T>
struct ValWithPGPercent {
    T value;
    float percentOfParent;
    float percentOfGlobal;
};

template<typename T>
struct ValWithGPercent {
    T value;
    float percentOfGlobal;
};

/**
 * View structures
 */
class APIRecord {
public:
    std::string symbolName = "";
    ValWithPGPercent<int64_t> duration;
    ValWithPGPercent<int64_t> count;

    uint64_t mean = 0; //For the measurement period only
    uint64_t variance = 0; //For the measurement period only
};


class LibRecord {
public:
    std::string libPath = "";
    ValWithPGPercent<int64_t> duration;
    ValWithPGPercent<int64_t> count;
    APIRecord *apiRecordArr;
};

class AppRecord {
public:
    std::string appPath = "";
    ValWithGPercent<int64_t> duration;
    int64_t totalDuration;
    std::vector<LibRecord> libRecordArr;
};


/**
 * Record structures
 */
struct RecSymInfo {
    std::string funcName;
    ssize_t fileId;
    ssize_t symIdInFile;
};

#endif
