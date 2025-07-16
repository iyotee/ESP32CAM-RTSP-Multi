/**
 * @file Logger.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Centralized log management module with verbosity levels
 */
// Logger.h
// Centralized log management module with verbosity levels

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Log levels
enum LogLevel
{
    LOG_ERROR = 0,  // Critical errors
    LOG_WARN = 1,   // Warnings
    LOG_INFO = 2,   // General information
    LOG_DEBUG = 3,  // Detailed debug
    LOG_VERBOSE = 4 // Very verbose debug
};

/**
 * @class Logger
 * @brief Provides centralized log management with different verbosity levels.
 *        Allows dynamic enabling/disabling of logs according to desired level.
 */
class Logger
{
public:
    // Log level configuration (default: INFO)
    static void setLogLevel(LogLevel level);
    static LogLevel getLogLevel();

    // Log methods with level
    static void error(const char *message);
    static void error(const String &message);
    static void warn(const char *message);
    static void warn(const String &message);
    static void info(const char *message);
    static void info(const String &message);
    static void debug(const char *message);
    static void debug(const String &message);
    static void verbose(const char *message);
    static void verbose(const String &message);

    // Log methods with formatting (printf style)
    static void errorf(const char *format, ...);
    static void warnf(const char *format, ...);
    static void infof(const char *format, ...);
    static void debugf(const char *format, ...);
    static void verbosef(const char *format, ...);

    // Utility methods
    static void printTimestamp();
    static void printLogLevel(LogLevel level);

private:
    static LogLevel currentLevel;
    static void log(LogLevel level, const char *message);
    static void logf(LogLevel level, const char *format, va_list args);

    // Prefixes for each level
    static const char *getLevelPrefix(LogLevel level);
};

// Macros to facilitate usage
#define LOG_ERROR(msg) Logger::error(msg)
#define LOG_WARN(msg) Logger::warn(msg)
#define LOG_INFO(msg) Logger::info(msg)
#define LOG_DEBUG(msg) Logger::debug(msg)
#define LOG_VERBOSE(msg) Logger::verbose(msg)

#define LOG_ERRORF(fmt, ...) Logger::errorf(fmt, ##__VA_ARGS__)
#define LOG_WARNF(fmt, ...) Logger::warnf(fmt, ##__VA_ARGS__)
#define LOG_INFOF(fmt, ...) Logger::infof(fmt, ##__VA_ARGS__)
#define LOG_DEBUGF(fmt, ...) Logger::debugf(fmt, ##__VA_ARGS__)
#define LOG_VERBOSEF(fmt, ...) Logger::verbosef(fmt, ##__VA_ARGS__)

#endif // LOGGER_H