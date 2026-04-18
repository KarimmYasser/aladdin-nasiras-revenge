#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace our {

    enum class LogLevel {
        Debug,      // Detailed information for debugging
        Info,       // General informational messages
        Warning,    // Warning messages
        Error,      // Error messages
        Critical    // Critical errors
    };

    /**
     * @brief A lightweight logging utility for the application.
     *
     * The Logger provides a simple, thread-safe logging interface with support for
     * multiple log levels, timestamps, and categories. It routes error messages to
     * stderr and info/debug messages to stdout.
     *
     * Usage:
     *   Logger::info("Component", "Message here");
     *   Logger::error("Component", "Error occurred:", value);
     *   Logger::setLogLevel(LogLevel::Debug);  // Show debug messages
     */
    class Logger {
    private:
        static LogLevel currentLevel;
        static std::string logLevelToString(LogLevel level);
        static std::string getTimestamp();

    public:
        /**
         * @brief Set the global logging level.
         * Messages below this level will be ignored.
         * Default is Info.
         */
        static void setLogLevel(LogLevel level) { currentLevel = level; }

        /**
         * @brief Log a message with the specified level and category.
         * Supports variadic arguments for flexible message construction.
         */
        template<typename... Args>
        static void log(LogLevel level, const std::string& category, const Args&... args) {
            if (static_cast<int>(level) < static_cast<int>(currentLevel)) {
                return;
            }

            std::ostringstream oss;
            oss << "[" << getTimestamp() << "] ";
            oss << "[" << logLevelToString(level) << "] ";
            oss << "[" << category << "] ";
            logArgs(oss, args...);

            if (level == LogLevel::Error || level == LogLevel::Critical) {
                std::cerr << oss.str() << std::endl;
            } else {
                std::cout << oss.str() << std::endl;
            }
        }

        // Convenience methods for different log levels

        /**
         * @brief Log a debug message.
         */
        template<typename... Args>
        static void debug(const std::string& category, const Args&... args) {
            log(LogLevel::Debug, category, args...);
        }

        /**
         * @brief Log an info message.
         */
        template<typename... Args>
        static void info(const std::string& category, const Args&... args) {
            log(LogLevel::Info, category, args...);
        }

        /**
         * @brief Log a warning message.
         */
        template<typename... Args>
        static void warning(const std::string& category, const Args&... args) {
            log(LogLevel::Warning, category, args...);
        }

        /**
         * @brief Log an error message.
         */
        template<typename... Args>
        static void error(const std::string& category, const Args&... args) {
            log(LogLevel::Error, category, args...);
        }

        /**
         * @brief Log a critical error message.
         */
        template<typename... Args>
        static void critical(const std::string& category, const Args&... args) {
            log(LogLevel::Critical, category, args...);
        }

    private:
        template<typename T>
        static void logArgs(std::ostringstream& oss, const T& arg) {
            oss << arg;
        }

        template<typename T, typename... Rest>
        static void logArgs(std::ostringstream& oss, const T& arg, const Rest&... rest) {
            oss << arg;
            logArgs(oss, rest...);
        }
    };

}

