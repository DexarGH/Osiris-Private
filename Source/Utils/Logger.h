#pragma once

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <ctime>
#include <sys/stat.h>

namespace Logger {

inline FILE* logFile = nullptr;
inline bool logInitialized = false;

inline void logRaw(const char* msg) {
    if (!logInitialized || !logFile) return;
    
    fseek(logFile, 0, SEEK_END);
    if (ftell(logFile) > 5 * 1024 * 1024) {
        fclose(logFile);
        logFile = nullptr;
        logInitialized = false;
        return;
    }
    
    fputs(msg, logFile);
    fflush(logFile);
}

inline void init() {
    if (logInitialized) return;
    
    mkdir("/home/typoi/OsirisCS2/logs", 0755);
    
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char path[256];
    snprintf(path, sizeof(path), "/home/typoi/OsirisCS2/logs/osiris_%04d-%02d-%02d.log",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    
    logFile = fopen(path, "a");
    if (logFile) {
        logInitialized = true;
        logRaw("[Logger] Log file opened\n");
    }
}

inline void log(const char* format, ...) {
    if (!logInitialized || !logFile) return;
    
    fseek(logFile, 0, SEEK_END);
    if (ftell(logFile) > 5 * 1024 * 1024) {
        fclose(logFile);
        logFile = nullptr;
        logInitialized = false;
        return;
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);
    fputc('\n', logFile);
    fflush(logFile);
}

inline void shutdown() {
    if (logFile) {
        logRaw("[Logger] Log file closed\n");
        fclose(logFile);
        logFile = nullptr;
        logInitialized = false;
    }
}

}

#define LOG(...) Logger::log(__VA_ARGS__)
#define LOG_INIT() Logger::init()
#define LOG_SHUTDOWN() Logger::shutdown()


