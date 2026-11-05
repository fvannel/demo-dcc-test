#include "dcc/decoder.hpp"

namespace dcc {

void Decoder::reset() {
    phase_ = Phase::preamble;
    packet_ = {};
    preamble_ = bits_ = byte_ = 0;
    half_pending_ = false;
}

bool Decoder::feed_halfbit(std::uint32_t us, Packet& output) {
    const bool one = us >= timing_.one_min_us && us <= timing_.one_max_us;
    const bool zero = us >= timing_.zero_min_us && us <= timing_.zero_max_us;
    if (!one && !zero) {
        ++stats_.timing_errors;
        reset();
        return false;
    }
    if (!half_pending_) {
        half_pending_ = true;
        first_half_one_ = one;
        return false;
    }
    half_pending_ = false;
    if (one != first_half_one_) {
        ++stats_.timing_errors;
        reset();
        // We may have started halfway through a bit. Reuse this half as the
        // beginning of the next pair so the receiver can recover alignment.
        half_pending_ = true;
        first_half_one_ = one;
        return false;
    }
    return feed_bit(one, output);
}

bool Decoder::feed_bit(bool one, Packet& output) {
    if (phase_ == Phase::preamble) {
        if (one) {
            if (preamble_ < 10) ++preamble_;
        } else {
            if (preamble_ >= 10) {
                phase_ = Phase::data;
                packet_ = {};
                byte_ = bits_ = 0;
            }
            preamble_ = 0;
        }
        return false;
    }
    if (phase_ == Phase::data) {
        byte_ = static_cast<std::uint8_t>((byte_ << 1) | (one ? 1 : 0));
        if (++bits_ == 8) {
            packet_.bytes[packet_.size++] = byte_;
            byte_ = bits_ = 0;
            phase_ = Phase::separator;
        }
        return false;
    }
    if (!one) {
        if (packet_.size == Packet::capacity) {
            ++stats_.framing_errors;
            reset();
        } else {
            phase_ = Phase::data;
        }
        return false;
    }
    bool valid = false;
    if (packet_.size < 3) {
        ++stats_.framing_errors;
    } else {
        std::uint8_t checksum = 0;
        for (std::uint8_t i = 0; i < packet_.size; ++i) checksum ^= packet_.bytes[i];
        if (checksum == 0) {
            output = packet_;
            ++stats_.accepted;
            valid = true;
        } else {
            ++stats_.checksum_errors;
        }
    }
    reset();
    // An end bit can also be the first bit of the next preamble.
    preamble_ = 1;
    return valid;
}

} // namespace dcc
