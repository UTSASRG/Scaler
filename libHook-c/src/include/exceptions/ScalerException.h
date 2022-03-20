#ifndef SCALER_SCALEREXCEPTION_H
#define SCALER_SCALEREXCEPTION_H

#include <exception>
#include <type/Object.hpp>
#include <string>
#include <iostream>
#include <util/tool/Logging.h>

#define fatalError(errMsg) \
    ERR_LOG(errMsg);        \
    exit(-1);

#define fatalErrorS(errFmt, ...) \
    ERR_LOGS(errFmt,__VA_ARGS__);            \
    exit(-1);

#endif
