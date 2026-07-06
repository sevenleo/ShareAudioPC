#include "app/Logger.h"

#include <chrono>
#include <iostream>
#include <mutex>

namespace shareaudio {
namespace {

std::mutex log_mutex;
LogLevel current_level = LogLevel::Info;

int level_rank(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return 0;
    case LogLevel::Info:
        return 1;
    case LogLevel::Warning:
        return 2;
    case LogLevel::Error:
        return 3;
    }
    return 1;
}

const char* level_name(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Info:
        return "info";
    case LogLevel::Warning:
        return "warning";
    case LogLevel::Error:
        return "error";
    }
    return "info";
}

} // namespace

void Logger::set_level(LogLevel level)
{
    std::scoped_lock lock(log_mutex);
    current_level = level;
}

void Logger::log(LogLevel level, std::string_view message)
{
    std::scoped_lock lock(log_mutex);
    if (level_rank(level) < level_rank(current_level)) {
        return;
    }

    std::cerr << '[' << level_name(level) << "] " << message << '\n';
}

void Logger::debug(std::string_view message)
{
    log(LogLevel::Debug, message);
}

void Logger::info(std::string_view message)
{
    log(LogLevel::Info, message);
}

void Logger::warning(std::string_view message)
{
    log(LogLevel::Warning, message);
}

void Logger::error(std::string_view message)
{
    log(LogLevel::Error, message);
}

} // namespace shareaudio
