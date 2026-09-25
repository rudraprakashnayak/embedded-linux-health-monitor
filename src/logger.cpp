#include "logger.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

Logger::Logger(std::string log_file, bool also_stderr)
    : log_file_(std::move(log_file)), also_stderr_(also_stderr) {}

void Logger::set_log_file(const std::string& path) {
    log_file_ = path;
}

const char* Logger::level_name(LogLevel level) {
    switch (level) {
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Critical:
            return "CRIT";
        default:
            return "INFO";
    }
}

void Logger::log(LogLevel level, const std::string& message) const {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream line;
    line << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [" << level_name(level) << "] "
         << message << '\n';
    const std::string text = line.str();

    if (also_stderr_) {
        std::clog << text;
    }

    if (log_file_.empty()) {
        return;
    }

    std::ofstream out(log_file_, std::ios::app);
    if (out) {
        out << text;
    }
}

void Logger::info(const std::string& message) const {
    log(LogLevel::Info, message);
}

void Logger::warning(const std::string& message) const {
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message) const {
    log(LogLevel::Error, message);
}

void Logger::critical(const std::string& message) const {
    log(LogLevel::Critical, message);
}
