#include <exceptions/ScalerException.h>
#include <util/tool/Logging.h>

//__thread int errorCode;
//thread_local std::string errorMsg;
//
//inline void setLastError(int code, std::string desc) {
//    errorCode = code;
//    errorMsg = desc;
//}
//
//inline const char *getLastErrorMsg() {
//    return errorMsg.c_str();
//}
//
//inline int getLastErrorCode() {
//    return errorCode;
//}