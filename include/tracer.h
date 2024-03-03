#ifndef ALE_TRACER
#define ALE_TRACER

// ext
#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <source_location>
#include <string_view>

#ifndef GLM
#define GLM
#include <glm/glm.hpp>
#endif // GLM

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
static void SetLogLevel(LogLevel lvl, bool isEnabled);
static void SetLogLevels(std::vector<LogLevel> lvls);


// Prints passed std::source_location object
static std::string _getLocation(const std::source_location loc) {
    std::string result = "  ~at: ";
    result.append(loc.file_name());
    result.append(":" + std::to_string(loc.line()) + " ");
    result.append(loc.function_name());

    return result;
}


// A switch statement dispatch for an ale::Tracer::LogLevel enum
static std::string _LogLevelToString(LogLevel lvl) {
     switch (lvl) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN_LOG_LEVEL";
    }
}

// Outputs a log with a certain LogLevel and a trace of where it was invoked
static void log(const std::string_view msg, LogLevel lvl,
                 const std::source_location loc) {
    constexpr char GRAY[] = "\033[90m";
    constexpr char RESET[] = "\033[0m";

    // Checks if the level is enabled
    if (!globalLogLevels[lvl]) {
        return;
    }
    std::cout << _LogLevelToString(lvl) << ": "
              << msg << "\n"
              << GRAY << _getLocation(loc) << RESET << "\n";

}

// This struct replaces std::cout functionality. You can add checks and formatting
struct Raw {};

extern Raw raw;

template <typename T>
Raw& operator<< (Raw &s, const T &x) {
  std::cout << x;
  return s;
}

// Set bool value for a log level (enabled when `true`)
[[maybe_unused]] static void SetLogLevel(LogLevel lvl, bool isEnabled) {

    if (lvl == LogLevel::LogLevel_MAX) {
        log("LogLevel_MAX is not a log level!", LogLevel::WARNING);
        return;
    }

    globalLogLevels[lvl] = isEnabled;
}



// Print a 4x4 matrix
[[maybe_unused]] static void printMat4(float* mat, std::string msg) {
    if (mat == nullptr) {
        log("Incorrect matrix input! Aborting logging", LogLevel::ERROR);
        return;
    }
    unsigned int mat_size = 4;
    raw << msg << "\n";
    for (size_t i = 0; i < mat_size; i++) {
        for (size_t j = 0; j < mat_size; j++) {
            raw << mat[i * mat_size + j] << "  ";
        }
        raw << "\n";
    }
    raw << "\n";

}

// Print a 4x4 matrix
// TODO: find a way to input glm matrices as templates for handling matrices of 
// arbitrary sizes
[[maybe_unused]] static void printMat4(const glm::mat4& mat, std::string msg) {
    raw << msg << "\n";
        for (size_t i = 0; i < 4; i++) {
            for (size_t j = 0; j < 4; j++)
            {
                raw << mat[i][j]<< "  ";
            }
            raw << "\n";
        }
        raw << "\n";
}

// Sets a list of used LogLevel's 
[[maybe_unused]] static void SetLogLevels(std::vector<LogLevel> lvls) {
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
