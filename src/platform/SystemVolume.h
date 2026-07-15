#pragma once

#include "app/Result.h"

#include <memory>
#include <string>

namespace shareaudio {

float gain_from_decibels(float decibels, bool muted);
bool system_volume_supported();

class SystemVolumeReader {
public:
    SystemVolumeReader();
    ~SystemVolumeReader();

    SystemVolumeReader(const SystemVolumeReader&) = delete;
    SystemVolumeReader& operator=(const SystemVolumeReader&) = delete;

    Result<void> bind(const std::wstring& endpoint_id);
    Result<float> read_gain(bool ignore_mute = false) const;
    Result<bool> read_muted() const;
    Result<void> set_muted(bool muted) const;
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace shareaudio
