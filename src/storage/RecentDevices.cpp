#include "storage/RecentDevices.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>

namespace shareaudio {

RecentDevices::RecentDevices(std::filesystem::path path, std::size_t limit)
    : path_(std::move(path))
    , limit_(limit)
{
}

Result<void> RecentDevices::load()
{
    entries_.clear();
    if (!std::filesystem::exists(path_)) {
        return Result<void>::success();
    }

    std::ifstream file(path_);
    if (!file) {
        return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to read recent devices file: " + path_.string()));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string json = buffer.str();
    const std::regex pattern("\"([^\"]+)\"");

    for (std::sregex_iterator it(json.begin(), json.end(), pattern), end; it != end; ++it) {
        const std::string value = (*it)[1].str();
        if (value != "recent_devices") {
            entries_.push_back(value);
        }
    }

    if (entries_.size() > limit_) {
        entries_.resize(limit_);
    }
    return Result<void>::success();
}

Result<void> RecentDevices::save() const
{
    try {
        if (path_.has_parent_path()) {
            std::filesystem::create_directories(path_.parent_path());
        }
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to create recent devices directory: " + std::string(error.what())));
    }

    std::ofstream file(path_);
    if (!file) {
        return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to write recent devices file: " + path_.string()));
    }

    file << "{\n  \"recent_devices\": [";
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        file << (i == 0 ? "\n    " : ",\n    ") << '"' << entries_[i] << '"';
    }
    if (!entries_.empty()) {
        file << '\n';
    }
    file << "  ]\n}\n";
    return Result<void>::success();
}

Result<void> RecentDevices::add(std::string host)
{
    if (host.empty()) {
        return Result<void>::failure(make_error(ErrorCode::InvalidArgument, "Recent device host cannot be empty."));
    }

    entries_.erase(std::remove(entries_.begin(), entries_.end(), host), entries_.end());
    entries_.insert(entries_.begin(), std::move(host));
    if (entries_.size() > limit_) {
        entries_.resize(limit_);
    }
    return save();
}

Result<void> RecentDevices::clear()
{
    entries_.clear();
    return save();
}

const std::vector<std::string>& RecentDevices::entries() const
{
    return entries_;
}

const std::filesystem::path& RecentDevices::path() const
{
    return path_;
}

std::filesystem::path default_recent_devices_path()
{
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata != nullptr) {
        return std::filesystem::path(appdata) / "ShareAudioLite" / "recent-devices.json";
    }
#else
    const char* home = std::getenv("HOME");
    if (home != nullptr) {
        return std::filesystem::path(home) / ".config" / "shareaudiolite" / "recent-devices.json";
    }
#endif
    return std::filesystem::temp_directory_path() / "shareaudiolite-recent-devices.json";
}

} // namespace shareaudio
