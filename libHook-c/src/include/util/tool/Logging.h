/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef SCALER_LOGGING_H
#define SCALER_LOGGING_H

#include <cstdio>

#define PRINT_INFO_LOG true
#define PRINT_DBG_LOG true
#define PRINT_ERR_LOG true
#if PRINT_DBG_LOG

// Print a single log string
#define DBG_LOG(str) fprintf(stdout,"DBG: %s:%d  ",__FILE__,__LINE__); fprintf(stdout,"%s\n",str)
// Print log strings using printf template format
#define DBG_LOGS(fmt, ...) fprintf(stdout,"DBG: %s:%d  ",__FILE__,__LINE__); fprintf(stdout,fmt,__VA_ARGS__); fprintf(stdout,"\n")
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
#define INFO_LOG(str) fprintf(stdout,"INFO: %s:%d  ",__FILE__,__LINE__); fprintf(stdout,"%s\n",str)
// Print log strings using printf template format
#define INFO_LOGS(fmt, ...) fprintf(stdout,"INFO: %s:%d  ",__FILE__,__LINE__); fprintf(stdout,fmt,__VA_ARGS__); fprintf(stdout,"\n")
// Print a single error string
#else

#define INFO_LOG(str)

#define INFO_LOGS(fmt, ...)

#endif


#endif //SCALER_FILETOOL_H
