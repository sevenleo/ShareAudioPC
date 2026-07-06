#pragma once

#include "app/Config.h"
#include "app/Result.h"

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace shareaudio {

struct AudioDevice {
    std::string id;
    std::string name;
    bool is_default {};
};

using PcmBytes = std::vector<std::uint8_t>;
using PcmCallback = std::function<void(std::span<const std::uint8_t>)>;

struct AudioStats {
    std::size_t bytes_processed {};
    std::size_t underruns {};
    std::size_t overruns {};
};

} // namespace shareaudio
