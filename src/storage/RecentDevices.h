#pragma once

#include "app/Result.h"

#include <filesystem>
#include <string>
#include <vector>

namespace shareaudio {

class RecentDevices {
public:
    explicit RecentDevices(std::filesystem::path path, std::size_t limit = 10);

    Result<void> load();
    Result<void> save() const;
    Result<void> add(std::string host);
    Result<void> clear();

    [[nodiscard]] const std::vector<std::string>& entries() const;
    [[nodiscard]] const std::filesystem::path& path() const;

private:
    std::filesystem::path path_;
    std::size_t limit_;
    std::vector<std::string> entries_;
};

std::filesystem::path default_recent_devices_path();

} // namespace shareaudio
