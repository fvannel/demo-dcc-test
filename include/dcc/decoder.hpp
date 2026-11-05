#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace dcc {

// Includes the final XOR byte. Longer formats need an explicit protocol extension.
struct Packet {
    static constexpr std::size_t capacity = 6;
    std::array<std::uint8_t, capacity> bytes{};
    std::uint8_t size = 0;
};

struct Timing {
    std::uint32_t one_min_us = 52;
    std::uint32_t one_max_us = 64;
    std::uint32_t zero_min_us = 90;
    std::uint32_t zero_max_us = 10000;
};

struct Statistics {
    std::uint32_t accepted = 0;
    std::uint32_t checksum_errors = 0;
    std::uint32_t framing_errors = 0;
    std::uint32_t timing_errors = 0;
};

// Single consumer: call only from the foreground loop, never from the ISR.
// No allocation, SDK dependency, hardware access or application side effects.
class Decoder {
public:
    explicit Decoder(Timing timing = {}) : timing_(timing) {}
    bool feed_halfbit(std::uint32_t duration_us, Packet& output);
    // Discard partial reception after capture loss or prolonged silence.
    // Statistics survive a reset. Output changes only on a valid complete packet.
    void reset();
    const Statistics& statistics() const { return stats_; }

private:
    enum class Phase { preamble, data, separator };
    bool feed_bit(bool one, Packet& output);
    Timing timing_;
    Statistics stats_{};
    Packet packet_{};
    Phase phase_ = Phase::preamble;
    std::uint8_t preamble_ = 0;
    std::uint8_t bits_ = 0;
    std::uint8_t byte_ = 0;
    bool half_pending_ = false;
    bool first_half_one_ = false;
};

} // namespace dcc
