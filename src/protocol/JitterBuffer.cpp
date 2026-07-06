#include "protocol/JitterBuffer.h"

#include <algorithm>

namespace shareaudio {

JitterBuffer::JitterBuffer(std::size_t capacity_bytes)
    : capacity_bytes_(capacity_bytes)
{
}

void JitterBuffer::push(std::span<const std::uint8_t> bytes)
{
    std::scoped_lock lock(mutex_);
    if (bytes.empty() || capacity_bytes_ == 0) {
        return;
    }

    const std::size_t incoming = bytes.size();
    if (incoming > capacity_bytes_) {
        ++overruns_;
        bytes_.clear();
        bytes_.insert(bytes_.end(), bytes.end() - static_cast<std::ptrdiff_t>(capacity_bytes_), bytes.end());
        return;
    }

    while (bytes_.size() + incoming > capacity_bytes_) {
        bytes_.pop_front();
        ++overruns_;
    }
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
}

std::vector<std::uint8_t> JitterBuffer::pop(std::size_t byte_count)
{
    std::scoped_lock lock(mutex_);
    const std::size_t available = std::min(byte_count, bytes_.size());
    std::vector<std::uint8_t> output;
    output.reserve(byte_count);

    for (std::size_t i = 0; i < available; ++i) {
        output.push_back(bytes_.front());
        bytes_.pop_front();
    }

    if (available < byte_count) {
        ++underruns_;
        output.resize(byte_count, 0);
    }
    return output;
}

void JitterBuffer::reset()
{
    std::scoped_lock lock(mutex_);
    bytes_.clear();
    underruns_ = 0;
    overruns_ = 0;
}

std::size_t JitterBuffer::depth_bytes() const
{
    std::scoped_lock lock(mutex_);
    return bytes_.size();
}

JitterBufferMetrics JitterBuffer::metrics() const
{
    std::scoped_lock lock(mutex_);
    return JitterBufferMetrics {
        bytes_.size(),
        capacity_bytes_,
        underruns_,
        overruns_
    };
}

} // namespace shareaudio
