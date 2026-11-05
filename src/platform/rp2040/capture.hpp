#pragma once

#include <cstdint>

namespace platform {

enum class CaptureResult { empty, duration, discontinuity };

// One instance, one core. Both GPIO interrupt and consumer run on core 0.
// The input must be an electrically conditioned 3.3 V logic signal.
void capture_init(unsigned gpio);
CaptureResult capture_pop(std::uint32_t& duration_us);

} // namespace platform
