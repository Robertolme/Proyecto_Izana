/**
 * @file Logger.hpp
 * @brief Simple logging utility for Proyecto Izana backend
 */

#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>

namespace izana {
namespace utils {

/**
 * @brief Simple singleton logger class
 */
class Logger {
public:
    enum class Level {
        DEBUG = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3
    };

    static Logger& instance();
    
    void set_level(Level level);
    void set_file(const std::string& filename);
    void set_console_enabled(bool enabled);
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    
    void log(Level level, const std::string& message);

private:
    Logger() = default;
    ~Logger() = default;
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void write_log(Level level, const std::string& message);
    std::string level_to_string(Level level) const;
    
    Level log_level_ = Level::INFO;
    bool console_enabled_ = true;
    std::unique_ptr<std::ofstream> log_file_;
    mutable std::mutex mutex_;
};

} // namespace utils
} // namespace izana