//
// Created by st on 1/10/21.
//

#ifndef SCALER_LOGGING_H
#define SCALER_LOGGING_H

#define PRINT_DBG_LOG true

#if PRINT_DBG_LOG

// Print a single log string
#define DBG_LOG(str) fprintf(stdout,"DBG: %s:%d  ",__FILE__,__LINE__); fprintf(stdout,"%s\n",str)
// Print log strings using printf template format
#define DBG_LOGS(fmt, ...) fprintf(stdout,"DBG: %s:%d  ",__FILE__,__LINE__); fprintf(stdout,fmt,__VA_ARGS__); fprintf(stdout,"\n")
// Print a single error string
#define ERR_LOG(str) fprintf(stderr,"ERR: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,"%s\n",str)
// Print a single error string with integer error code
#define ERR_LOGC(str, code) fprintf(stderr,"ERR: %s:%d ErrCode=%d  ",__FILE__,__LINE__,code); fprintf(stderr,"%s\n",str)
// Print log strings using printf template format
#define ERR_LOGS(fmt, ...) fprintf(stderr,"ERR: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,fmt,__VA_ARGS__);fprintf(stderr,"\n")

#else

// Print a single log string
#define DBG_LOG(str)
// Print log strings using printf template format
#define DBG_LOGS(fmt, ...)
// Print a single error string
#define ERR_LOG(str) fprintf(stderr,"ERR: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,"%s\n",str)
// Print a single error string with integer error code
#define ERR_LOGC(str, code)  fprintf(stderr,"ERR: %s:%d ErrCode=%d  ",__FILE__,__LINE__,code); fprintf(stderr,"%s\n",str)
// Print log strings using printf template format
#define ERR_LOGS(fmt, ...) fprintf(stderr,"ERR: %s:%d  ",__FILE__,__LINE__); fprintf(stderr,fmt,__VA_ARGS__);fprintf(stderr,"\n")

#endif

#endif //SCALER_FILETOOL_H
