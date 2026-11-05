#include "capture.hpp"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/time.h"

namespace {
constexpr std::uint16_t capacity = 256;
volatile std::uint32_t durations[capacity]{};
volatile std::uint16_t head = 0;
volatile std::uint16_t tail = 0;
volatile bool lost = false;
unsigned input_gpio = 0;
std::uint32_t last_edge = 0;
bool first_edge = true;

void on_edge(unsigned gpio, std::uint32_t events) {
    if (gpio != input_gpio) return;
    const auto now = time_us_32();
    // Both flags mean at least one edge may have coalesced while IRQs were masked.
    if ((events & GPIO_IRQ_EDGE_RISE) && (events & GPIO_IRQ_EDGE_FALL)) {
        lost = true;
        last_edge = now;
        first_edge = false;
        return;
    }
    if (first_edge) {
        last_edge = now;
        first_edge = false;
        return;
    }
    const std::uint32_t duration = now - last_edge;
    last_edge = now;
    const auto next = static_cast<std::uint16_t>((head + 1) % capacity);
    if (next == tail) {
        lost = true;
        return;
    }
    durations[head] = duration;
    head = next;
}
} // namespace

namespace platform {
void capture_init(unsigned gpio) {
    input_gpio = gpio;
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_disable_pulls(gpio);
    gpio_set_irq_enabled_with_callback(gpio, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                       true, &on_edge);
}

CaptureResult capture_pop(std::uint32_t& duration_us) {
    const auto irq_state = save_and_disable_interrupts();
    CaptureResult result = CaptureResult::empty;
    if (lost) {
        tail = head;
        lost = false;
        result = CaptureResult::discontinuity;
    } else if (tail != head) {
        duration_us = durations[tail];
        tail = static_cast<std::uint16_t>((tail + 1) % capacity);
        result = CaptureResult::duration;
    }
    restore_interrupts(irq_state);
    return result;
}
} // namespace platform
