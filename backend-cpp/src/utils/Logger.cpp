/**
 * @file Logger.cpp
 * @brief Implementation of logging utility
 */

#include "utils/Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace izana {
namespace utils {

Logger& Logger::instance() {
    static Logger instance;
    return instance;
}

void Logger::set_level(Level level) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_level_ = level;
}

void Logger::set_file(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_file_ = std::make_unique<std::ofstream>(filename, std::ios::app);
}

void Logger::set_console_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    console_enabled_ = enabled;
}

void Logger::debug(const std::string& message) {
    log(Level::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(Level::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(Level::WARN, message);
}

void Logger::error(const std::string& message) {
    log(Level::ERROR, message);
}

void Logger::log(Level level, const std::string& message) {
    if (level < log_level_) {
        return;
    }
    
    write_log(level, message);
}

void Logger::write_log(Level level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    ss << " [" << level_to_string(level) << "] " << message;
    
    std::string formatted_message = ss.str();
    
    // Write to console
    if (console_enabled_) {
        std::cout << formatted_message << std::endl;
    }
    
    // Write to file
    if (log_file_ && log_file_->is_open()) {
        *log_file_ << formatted_message << std::endl;
        log_file_->flush();
    }
}

std::string Logger::level_to_string(Level level) const {
    switch (level) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO ";
        case Level::WARN:  return "WARN ";
        case Level::ERROR: return "ERROR";
        default:           return "UNKNW";
    }
}