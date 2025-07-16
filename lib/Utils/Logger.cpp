/**
 * @file Logger.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Implementation of centralized log management module
 */
// Logger.cpp
// Implementation of centralized log management module

#include "Logger.h"
#include <stdarg.h>
#include "../../src/config.h"

// Default log level initialization
LogLevel Logger::currentLevel = LOG_INFO;

void Logger::setLogLevel(LogLevel level)
{
    currentLevel = level;
    infof("Log level set to: %d", level);
}

LogLevel Logger::getLogLevel()
{
    return currentLevel;
}

// Log methods with level
void Logger::error(const char *message)
{
    log(LOG_ERROR, message);
}

void Logger::error(const String &message)
{
    error(message.c_str());
}

void Logger::warn(const char *message)
{
    log(LOG_WARN, message);
}

void Logger::warn(const String &message)
{
    warn(message.c_str());
}

void Logger::info(const char *message)
{
    log(LOG_INFO, message);
}

void Logger::info(const String &message)
{
    info(message.c_str());
}

void Logger::debug(const char *message)
{
    log(LOG_DEBUG, message);
}

void Logger::debug(const String &message)
{
    debug(message.c_str());
}

void Logger::verbose(const char *message)
{
    log(LOG_VERBOSE, message);
}

void Logger::verbose(const String &message)
{
    verbose(message.c_str());
}

// Log methods with formatting
void Logger::errorf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logf(LOG_ERROR, format, args);
    va_end(args);
}

void Logger::warnf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logf(LOG_WARN, format, args);
    va_end(args);
}

void Logger::infof(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logf(LOG_INFO, format, args);
    va_end(args);
}

void Logger::debugf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[LOG_BUFFER_SIZE];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    logf(LOG_DEBUG, format, args);
}

void Logger::verbosef(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    logf(LOG_VERBOSE, format, args);
    va_end(args);
}

// Utility methods
void Logger::printTimestamp()
{
    unsigned long uptime = millis();
    Serial.printf("[%lu.%03lu] ", uptime / 1000, uptime % 1000);
}

void Logger::printLogLevel(LogLevel level)
{
    Serial.print(getLevelPrefix(level));
}

// Private methods
void Logger::log(LogLevel level, const char *message)
{
    if (level <= currentLevel)
    {
        printTimestamp();
        printLogLevel(level);
        Serial.println(message);
    }
}

void Logger::logf(LogLevel level, const char *format, va_list args)
{
    if (level <= currentLevel)
    {
        printTimestamp();
        printLogLevel(level);

        // Create temporary buffer for formatted message
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        Serial.println(buffer);
    }
}

const char *Logger::getLevelPrefix(LogLevel level)
{
    switch (level)
    {
    case LOG_ERROR:
        return "[ERROR] ";
    case LOG_WARN:
        return "[WARN]  ";
    case LOG_INFO:
        return "[INFO]  ";
    case LOG_DEBUG:
        return "[DEBUG] ";
    case LOG_VERBOSE:
        return "[VERB]  ";
    default:
        return "[UNKN]  ";
    }
}