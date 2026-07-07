#include "protocol/PcmChunker.h"

#include <algorithm>

namespace shareaudio {

PcmChunker::PcmChunker(AudioMode mode)
    : packet_size_(packet_size_for_mode(mode))
{
}

void PcmChunker::push(std::span<const std::uint8_t> bytes)
{
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

bool PcmChunker::has_packet() const
{
    return packet_size_ > 0 && buffer_.size() >= packet_size_;
}

std::vector<std::uint8_t> PcmChunker::pop_packet()
{
    if (!has_packet()) {
        return {};
    }

    std::vector<std::uint8_t> packet;
    packet.reserve(packet_size_);
    for (std::size_t i = 0; i < packet_size_; ++i) {
        packet.push_back(buffer_.front());
        buffer_.pop_front();
    }
    return packet;
}

std::size_t PcmChunker::buffered_bytes() const
{
    return buffer_.size();
}

std::size_t PcmChunker::packet_size() const
{
    return packet_size_;
}

void PcmChunker::reset()
{
    buffer_.clear();
}

} // namespace shareaudio
