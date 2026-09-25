#pragma once

#include <string>

enum class LogLevel {
    Info,
    Warning,
    Error,
    Critical
};

class Logger {
public:
    explicit Logger(std::string log_file = "", bool also_stderr = true);

    void set_log_file(const std::string& path);
    void log(LogLevel level, const std::string& message) const;

    void info(const std::string& message) const;
    void warning(const std::string& message) const;
    void error(const std::string& message) const;
    void critical(const std::string& message) const;

private:
    std::string log_file_;
    bool also_stderr_;

    static const char* level_name(LogLevel level);
};
