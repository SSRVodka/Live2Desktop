/**
 * @file   logger.h
 * @brief  The logging utilities.
 * 
 * @author SSRVodka
 * @date   Jan 30, 2024
 */

#pragma once

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

#include "utils/consts.h"

#ifdef APP_LOG_LEVEL_ERROR
    #define _ENABLE_ERROR
#elif defined(APP_LOG_LEVEL_WARN)
    #define _ENABLE_ERROR
    #define _ENABLE_WARN
#elif defined(APP_LOG_LEVEL_INFO)
    #define _ENABLE_ERROR
    #define _ENABLE_WARN
    #define _ENABLE_INFO
#elif defined(APP_LOG_LEVEL_DEBUG)
    #define _ENABLE_ERROR
    #define _ENABLE_WARN
    #define _ENABLE_INFO
    #define _ENABLE_DEBUG
#elif defined(APP_LOG_LEVEL_VERBOSE)
    #define _ENABLE_ERROR
    #define _ENABLE_WARN
    #define _ENABLE_INFO
    #define _ENABLE_DEBUG
    #define _ENABLE_VERBOSE
#endif

class Logger {
public:
    /**
     * @brief The log manager constructor.
     * 
     * @param destFn The output file for the log (default(NULL) is `stdout`).
     */
    Logger(const char* destFn=NULL);
    ~Logger();
    /** @brief Log event in debug level. */
    #define Debug(msg) _debug(msg, __FILE__, __LINE__)
    void _debug(const char* msg, const char* fn, int lineno) const;
    void _debug(const std::string &msg, const char *fn, int lineno) const;
    /** @brief Log event in info level. */
    #define Info(msg) _info(msg, __FILE__, __LINE__)
    void _info(const char* msg, const char* fn, int lineno) const;
    void _info(const std::string &msg, const char *fn, int lineno) const;
    /** @brief Log event in warning level. */
    #define Warning(msg) _warning(msg, __FILE__, __LINE__)
    void _warning(const char* msg, const char* fn, int lineno) const;
    void _warning(const std::string &msg, const char *fn, int lineno) const;
    /** @brief Log event in error level. */
    #define Exception(msg) _error(msg, __FILE__, __LINE__)
    void _error(const char* msg, const char* fn, int lineno) const;
    void _error(const std::string &msg, const char *fn, int lineno) const;
    /** @brief Log event for test. */
    #define Test(msg) _test(msg, __FILE__, __LINE__)
    void _test(const char* msg, const char* fn, int lineno) const;
    void _test(const std::string &msg, const char *fn, int lineno) const;

    #define Verbose(msg) _verbose(msg, __FILE__, __LINE__)
    void _verbose(const char* msg, const char* fn, int lineno) const;
    void _verbose(const std::string &msg, const char *fn, int lineno) const;

private:
    const char* destFn;
    FILE* dest;
};

const Logger stdLogger;
