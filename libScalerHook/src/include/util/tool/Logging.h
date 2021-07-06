//
// Created by st on 1/10/21.
//

#ifndef SCALER_LOGGING_H
#define SCALER_LOGGING_H

#define PRINT_DBG_LOG true

#if PRINT_DBG_LOG

// Print a single log string
#define DBG_LOG(str) fprintf(stdout,"DBG: %s:%d %s\n",__FILE__,__LINE__,str);
// Print log strings using printf template format
#define DBG_LOGS(fmt, ...) fprintf(stdout,fmt,__VA_ARGS__);
// Print a single error string
#define ERR_LOG(str) fprintf(stderr,"ERR: %s:%d %s\n",__FILE__,__LINE__,str);
// Print a single error string with integer error code
#define ERR_LOGC(str, code) fprintf(stderr,"ERR: Code=%d %s:%d %s\n",code,__FILE__,__LINE__,str);
// Print log strings using printf template format
#define ERR_LOGS(fmt, ...) fprintf(stderr,fmt,__VA_ARGS__);


#else

// Print a single log string
#define DBG_LOG(str)
// Print log strings using printf template format
#define DBG_LOGS(fmt, ...)
// Print a single error string
#define ERR_LOG(str)
// Print a single error string with integer error code
#define ERR_LOGC(str, code)
// Print log strings using printf template format
#define ERR_LOGS(fmt, ...)

#endif

#endif //SCALER_FILETOOL_H
