#include "capture.hpp"
#include "dcc/decoder.hpp"
#include "pico/stdlib.h"

namespace {
constexpr unsigned dcc_input_gpio = 2;
// Observable with a debugger; this scaffold deliberately drives no load.
volatile std::uint32_t valid_packets = 0;
volatile std::uint32_t capture_losses = 0;
}

int main() {
    dcc::Decoder decoder;
    dcc::Packet packet;
    platform::capture_init(dcc_input_gpio);
    auto last_sample = time_us_32();
    while (true) {
        // Bounded work leaves time for the future safety and output tasks.
        for (unsigned budget = 0; budget < 64; ++budget) {
            std::uint32_t duration = 0;
            const auto result = platform::capture_pop(duration);
            if (result == platform::CaptureResult::empty) break;
            if (result == platform::CaptureResult::discontinuity) {
                ++capture_losses;
                decoder.reset();
                continue;
            }
            last_sample = time_us_32();
            if (decoder.feed_halfbit(duration, packet)) {
                ++valid_packets;
                // Next boundary: address filter -> instruction parser -> application.
            }
        }
        if (static_cast<std::uint32_t>(time_us_32() - last_sample) > 30000) {
            decoder.reset();
        }
        sleep_us(20);
    }
}
