#include "logger.hpp"

namespace our {

    LogLevel Logger::currentLevel = LogLevel::Info;

    std::string Logger::logLevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug:    return "DEBUG";
            case LogLevel::Info:     return "INFO";
            case LogLevel::Warning:  return "WARN";
            case LogLevel::Error:    return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }

    std::string Logger::getTimestamp() {
        auto now = std::time(nullptr);
        struct tm timeInfo;

#ifdef _WIN32
        localtime_s(&timeInfo, &now);
#else
        localtime_r(&now, &timeInfo);
#endif

        std::ostringstream oss;
        oss << std::put_time(&timeInfo, "%H:%M:%S");
        return oss.str();
    }

}

