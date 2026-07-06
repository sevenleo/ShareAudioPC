#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <span>
#include <vector>

namespace shareaudio {

struct JitterBufferMetrics {
    std::size_t depth_bytes {};
    std::size_t capacity_bytes {};
    std::size_t underruns {};
    std::size_t overruns {};
};

class JitterBuffer {
public:
    explicit JitterBuffer(std::size_t capacity_bytes);

    void push(std::span<const std::uint8_t> bytes);
    std::vector<std::uint8_t> pop(std::size_t byte_count);
    void reset();

    [[nodiscard]] std::size_t depth_bytes() const;
    [[nodiscard]] JitterBufferMetrics metrics() const;

private:
    mutable std::mutex mutex_;
    std::deque<std::uint8_t> bytes_;
    std::size_t capacity_bytes_ {};
    std::size_t underruns_ {};
    std::size_t overruns_ {};
};

} // namespace shareaudio
