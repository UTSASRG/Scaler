#ifndef SCALER_LOGGING_H
#define SCALER_LOGGING_H

#include <cstdio>

#define PRINT_INFO_LOG true
#define PRINT_DBG_LOG true
#define PRINT_ERR_LOG true
#if PRINT_DBG_LOG

// Print a single log string
#define DBG_LOG(str) fprintf(stderr,"DBG: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,"%s\n",str)
// Print log strings using printf template format
#define DBG_LOGS(fmt, ...) fprintf(stderr,"DBG: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,fmt,__VA_ARGS__); fprintf(stderr,"\n")
// Print a single error string

#else

// Print a single log string
#define DBG_LOG(str)
// Print log strings using printf template format
#define DBG_LOGS(fmt, ...)
// Print a single error string

#endif

#if PRINT_ERR_LOG


#define ERR_LOG(str) fprintf(stderr,"ERR: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,"%s\n",str)
// Print a single error string with integer error code
#define ERR_LOGC(str, code) fprintf(stderr,"ERR: %s:%d ErrCode=%d  ",__FILE__,__LINE__,code); fprintf(stderr,"%s\n",str)
// Print log strings using printf template format
#define ERR_LOGS(fmt, ...) fprintf(stderr,"ERR: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,fmt,__VA_ARGS__);fprintf(stderr,"\n")

#else

#define ERR_LOG(str)
// Print a single error string with integer error code
#define ERR_LOGC(str, code)
// Print log strings using printf template format
#define ERR_LOGS(fmt, ...)

#endif

#if PRINT_INFO_LOG

// Print a single log string
#define INFO_LOG(str) fprintf(stderr,"INFO: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,"%s\n",str)
// Print log strings using printf template format
#define INFO_LOGS(fmt, ...) fprintf(stderr,"INFO: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,fmt,__VA_ARGS__); fprintf(stderr,"\n")
// Print a single error string

#else

// Print a single log string
#define INFO_LOG(str)
// Print log strings using printf template format
#define INFO_LOGS(fmt, ...)
// Print a single error string

#endif


#endif //SCALER_LOGGING_H
