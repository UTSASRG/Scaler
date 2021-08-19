#if !defined(DOUBLETAKE_LOG_H)
#define DOUBLETAKE_LOG_H

/*
 * @file:   log.h
 * @brief:  Logging and debug printing macros
 *          Color codes from SNAPPLE: http://sourceforge.net/projects/snapple/
 * @author: Charlie Curtsinger & Tongping Liu
 */

#include <stdio.h>
#include <string.h>

#include "xdefines.hh"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

#define NORMAL_CYAN "\033[36m"
#define NORMAL_MAGENTA "\033[35m"
#define NORMAL_BLUE "\033[34m"
#define NORMAL_YELLOW "\033[33m"
#define NORMAL_GREEN "\033[32m"
#define NORMAL_RED "\033[31m"

#define BRIGHT_CYAN "\033[1m\033[36m"
#define BRIGHT_MAGENTA "\033[1m\033[35m"
#define BRIGHT_BLUE "\033[1m\033[34m"
#define BRIGHT_YELLOW "\033[1m\033[33m"
#define BRIGHT_GREEN "\033[1m\033[32m"
#define BRIGHT_RED "\033[1m\033[31m"

#define ESC_INF NORMAL_CYAN
#define ESC_DBG NORMAL_GREEN
#define ESC_WRN BRIGHT_YELLOW
#define ESC_ERR BRIGHT_RED
#define ESC_END "\033[0m"


//#define OUTPUT
#define OUTPUT Real::write

#ifndef NDEBUG
/**
 * Print status-information message: level 0
 */
#define PRINF(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 1) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_INF "%lx [INFO]: %20s:%-4d: " fmt ESC_END "\n", pthread_self(),    \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }

/**
 * Print status-information message: level 1
 */
#define PRDBG(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 2) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_DBG "%lx [DBG]: %20s:%-4d: " fmt ESC_END "\n", pthread_self(),     \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }

/**
 * Print warning message: level 2
 */
#define PRWRN(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 3) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_WRN "%lx [WRN]: %20s:%-4d: " fmt ESC_END, pthread_self(),     \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }

/**
 * Print status-information message: level 3
 */
#define RCA_PRDBG(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 4) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_DBG "%lx [RCA_DBG]: %26s:%-4d: " fmt ESC_END, pthread_self(),     \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }
/**
 * Print status-information message: level 4
 */
#define RCA_PRINF(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 5) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_INF "%lx [RCA_INFO]: %26s:%-4d: " fmt ESC_END, pthread_self(),    \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }
/**
 * Print warning message: level 5
 */
#define RCA_PRWRN(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 6) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_WRN "%lx [RCA_WRN]: %26s:%-4d: " fmt ESC_END "\n", pthread_self(),     \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }
#else
#define PRINF(fmt, ...)
#define PRDBG(fmt, ...)
#define PRWRN(fmt, ...)
#define RCA_PRINF(fmt, ...)
#define RCA_PRDBG(fmt, ...)
#define RCA_PRWRN(fmt, ...)
#endif

/**
 * Print error message: level 2
 */
#define PRERR(fmt, ...)                                                                            \
  {                                                                                                \
    if(DEBUG_LEVEL < 4) {                                                                          \
      ::snprintf(getThreadBuffer(), LOG_SIZE,                                                      \
                 ESC_ERR "%lx [ERR]: %20s:%-4d: " fmt ESC_END "\n", pthread_self(),     \
                 __FILE__, __LINE__, ##__VA_ARGS__);                                               \
      OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                 \
    }                                                                                              \
  }

// Can't be turned off. But we don't want to output those line number information.
#define PRINT(fmt, ...)                                                                            \
  {                                                                                                \
    ::snprintf(getThreadBuffer(), LOG_SIZE, BRIGHT_MAGENTA fmt ESC_END "\n", ##__VA_ARGS__);       \
    OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                   \
  }

/**
 * Print fatal error message, the program is going to exit.
 */

#define FATAL(fmt, ...)                                                                            \
  {                                                                                                \
    ::snprintf(getThreadBuffer(), LOG_SIZE,                                                        \
               ESC_ERR "%lx [FATALERROR]: %20s:%-4d: " fmt ESC_END "\n",                \
               pthread_self(), __FILE__, __LINE__, ##__VA_ARGS__);                                 \
    exit(-1);                                                                                      \
    OUTPUT(OUTFD, getThreadBuffer(), strlen(getThreadBuffer()));                                   \
  }

// Check a condition. If false, print an error message and abort
#define REQUIRE(cond, ...)                                                                         \
  if(!(cond)) {                                                                                    \
    FATAL(__VA_ARGS__)                                                                             \
  }

#endif
