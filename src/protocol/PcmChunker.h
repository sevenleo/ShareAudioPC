#pragma once

#include "app/Config.h"

#include <cstdint>
#include <deque>
#include <span>
#include <vector>

namespace shareaudio {

class PcmChunker {
public:
    explicit PcmChunker(AudioMode mode);
    explicit PcmChunker(std::size_t packet_size);

    void push(std::span<const std::uint8_t> bytes);
    [[nodiscard]] bool has_packet() const;
    [[nodiscard]] std::vector<std::uint8_t> pop_packet();
    [[nodiscard]] std::size_t buffered_bytes() const;
    [[nodiscard]] std::size_t packet_size() const;
    void reset();

private:
    std::size_t packet_size_ {};
    std::deque<std::uint8_t> buffer_;
};

} // namespace shareaudio
