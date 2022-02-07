#ifndef SCALER_SCALEREXCEPTION_H
#define SCALER_SCALEREXCEPTION_H

#include <exception>
#include <type/Object.hpp>
#include <string>
#include <iostream>
#include <util/tool/Logging.h>
//Please comment fprintf clause when not debugging

//#define throwScalerExceptionS(code, errFmt, ...) \
//    char errMsg[1024];                \
//    sprintf(errMsg,errFmt,__VA_ARGS__);          \
//    ERR_LOG(errMsg);                             \
//    throw ScalerException(std::string(errMsg),0, __FILE__, __LINE__)

//#define throwScalerException(code, errFmt) \
//    char errMsg[1024];                \
//    sprintf(errMsg,errFmt);                \
//    ERR_LOG(errMsg); \
//    throw ScalerException(std::string(errMsg),0, __FILE__, __LINE__)

#define fatalError(errMsg) \
    ERR_LOG(errMsg);        \
    exit(-1);

#define fatalErrorS(errFmt, ...) \
    ERR_LOGS(errFmt,__VA_ARGS__);            \
    exit(-1);

class ScalerException : std::runtime_error, Object {
public:
    std::string info;
    int code;

    ScalerException(const std::string &info, int code, const std::string &fileName, int lineNo) : runtime_error(info),
                                                                                                  fileName(fileName),
                                                                                                  lineNo(lineNo) {
        this->info = info;
        this->code = code;
        this->fileName = fileName;
        this->lineNo = lineNo;

    }

    const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
        return this->info.c_str();
    }

private:
    std::string fileName;
    int lineNo;
};

#endif
