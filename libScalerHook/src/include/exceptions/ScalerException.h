#ifndef SCALER_SCALEREXCEPTION_H
#define SCALER_SCALEREXCEPTION_H

#include <exception>
#include <type/Object.hpp>
#include <string>
#include <iostream>

//Please comment fprintf clause when not debugging

#define throwScalerException(msg) \
    fprintf(stderr, "Scalar Exception thrown @ %s:%d: %s\n", __FILE__, __LINE__,msg); \
    throw ScalerException(msg,0, __FILE__, __LINE__);

#define throwScalerExceptionWithCode(msg, code) \
    fprintf(stderr, "Scalar Exception thrown @ %s:%d: %s\n", __FILE__, __LINE__,msg); \
    throw ScalerException(msg,code, __FILE__, __LINE__);

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
