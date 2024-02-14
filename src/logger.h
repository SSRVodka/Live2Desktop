/**
 * @file   logger.h
 * @brief  The logging utilities.
 * 
 * @author SSRVodka
 * @date   Jan 30, 2024
 */

#pragma once

#include <assert.h>
#include <stdio.h>


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
    /** @brief Log event in info level. */
    #define Info(msg) _info(msg, __FILE__, __LINE__)
    void _info(const char* msg, const char* fn, int lineno) const;
    /** @brief Log event in warning level. */
    #define Warning(msg) _warning(msg, __FILE__, __LINE__)
    void _warning(const char* msg, const char* fn, int lineno) const;
    /** @brief Log event in error level. */
    #define Exception(msg) _error(msg, __FILE__, __LINE__)
    void _error(const char* msg, const char* fn, int lineno) const;

private:
    const char* destFn;
    FILE* dest;
};

const Logger stdLogger;
