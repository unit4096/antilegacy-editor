#ifndef ALE_TRACER
#define ALE_TRACER

// ext
#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <source_location>
#include <string_view>

// int

namespace ale {

namespace Tracer {

enum LogLevel {
    INFO,
    DEBUG,
    WARNING,
    ERROR,
    LogLevel_MAX,
};

static std::string _getLocation(const std::source_location loc);
static std::string _LogLevelToString(LogLevel lvl);

static std::vector<bool> globalLogLevels = {true, true, true, true, true};
static void log(const std::string_view msg, LogLevel lvl = LogLevel::DEBUG, const std::source_location loc = std::source_location::current());
static void logRaw(const std::string_view msg);
static void SetLogLevel(LogLevel lvl, bool isEnabled);
static void SetLogLevels(std::vector<LogLevel> lvls);


// Prints passed source_location object
static std::string _getLocation(const std::source_location loc) {
    std::string result = " [Location: ";
    result.append(loc.file_name());
    result.append(":" + std::to_string(loc.line()) + ":"
                  + std::to_string(loc.column()) + " ");
    result.append(loc.function_name());
    result.append("] \n");

    return result;
}


// A switch statement dispatch for ale::LogLevel
static std::string _LogLevelToString(LogLevel lvl) {
     switch (lvl) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default: return "CANNOT_RECOGNIZE";
    }
}


static void log(const std::string_view msg, LogLevel lvl,
                 const std::source_location loc) {

    std::cout << globalLogLevels[lvl] << std::endl;

    // Checks if the level is enabled
    if (!globalLogLevels[lvl]) {
        return;
    }
    
    std::cout << _LogLevelToString(lvl) << ": " 
              << msg 
              << _getLocation(loc) << "\n";
    
}


//Prints input string without a newline
static void logRaw(const std::string_view msg) {
    std::cout << msg;
}

// Set bool value for a log level (enabled when `true`)
static void SetLogLevel(LogLevel lvl, bool isEnabled) {

    if (lvl == LogLevel::LogLevel_MAX) {
        log("LogLevel_MAX is not a log level!", LogLevel::WARNING);
        return;
    }

    globalLogLevels[lvl] = isEnabled;
}


static void SetLogLevels(std::vector<LogLevel> lvls) {
    globalLogLevels.resize(LogLevel::LogLevel_MAX);
    for (auto lvl: globalLogLevels) {
        lvl = false;
    }
    // Enables only specified levels
    for (auto lvl: lvls) {
        globalLogLevels[lvl] = true;
    }
}


} // namespace Tracer

}//namespace ale

#endif //ALE_TRACER