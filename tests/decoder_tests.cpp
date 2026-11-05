#include "dcc/decoder.hpp"

#include <cstdlib>
#include <initializer_list>
#include <iostream>

namespace {
void check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
struct Receiver {
    dcc::Decoder decoder;
    dcc::Packet packet;
    unsigned delivered = 0;
    void half(std::uint32_t us) {
        if (decoder.feed_halfbit(us, packet)) ++delivered;
    }
    void bit(bool one) { half(one ? 58 : 100); half(one ? 58 : 100); }
    void frame(std::initializer_list<std::uint8_t> bytes, unsigned preamble = 14) {
        for (unsigned i = 0; i < preamble; ++i) bit(true);
        bit(false);
        unsigned count = 0;
        for (const auto byte : bytes) {
            for (int i = 7; i >= 0; --i) bit((byte >> i) & 1);
            bit(++count == bytes.size());
        }
    }
};
}

int main() {
    {
        Receiver r;
        r.frame({3, 0x3f, 0x82, 0xbe});
        check(r.delivered == 1 && r.packet.size == 4 && r.packet.bytes[2] == 0x82,
              "four-byte packet assembled MSB first with checksum");
        r.frame({0xff, 0, 0xff}, 9);
        check(r.delivered == 2, "end bit contributes to next ten-bit preamble");
    }
    {
        Receiver r;
        r.packet.size = 42;
        r.frame({3, 0x3f, 0x82, 0});
        check(r.delivered == 0 && r.decoder.statistics().checksum_errors == 1,
              "bad checksum rejected");
        check(r.packet.size == 42, "rejected data does not overwrite output");
        r.frame({0xff, 0, 0xff});
        check(r.delivered == 1, "recovery after checksum failure");
    }
    {
        Receiver r;
        r.frame({0xff, 0, 0xff}, 9);
        check(r.delivered == 0, "short preamble rejected");
        r.decoder.reset();
        r.frame({1, 1});
        check(r.delivered == 0 && r.decoder.statistics().framing_errors == 1,
              "too-short packet rejected");
    }
    {
        Receiver r;
        r.frame({1, 2, 3, 4, 5, 1});
        check(r.delivered == 1 && r.packet.size == 6, "maximum length accepted");
        r.frame({1, 2, 3, 4, 5, 6, 7});
        check(r.delivered == 1 && r.decoder.statistics().framing_errors == 1,
              "oversized packet rejected without buffer overflow");
        r.frame({0xff, 0, 0xff});
        check(r.delivered == 2, "recovery after oversized packet");
    }
    {
        Receiver r;
        for (unsigned i = 0; i < 14; ++i) r.bit(true);
        r.bit(false);
        r.half(58); r.half(100);
        r.half(20); r.half(10001);
        check(r.decoder.statistics().timing_errors == 3, "mismatched/invalid halves rejected");
        r.frame({0xff, 0, 0xff});
        check(r.delivered == 1, "recovery after timing errors");
    }
    {
        Receiver r;
        for (unsigned i = 0; i < 10; ++i) { r.half(52); r.half(64); }
        r.bit(false);
        r.decoder.reset();
        r.frame({0xff, 0, 0xff}, 9);
        check(r.delivered == 0, "capture discontinuity clears partial state");
        check(r.decoder.statistics().timing_errors == 0, "one timing boundaries accepted");
        r.frame({0xff, 0, 0xff});
        check(r.delivered == 1, "fresh frame accepted after reset");
    }
    {
        Receiver r;
        r.half(58); // Capture begins in the middle of a preamble bit.
        r.frame({0xff, 0, 0xff});
        r.frame({3, 0x3f, 0x82, 0xbe});
        check(r.delivered >= 1 && r.packet.bytes[0] == 3,
              "reacquire half-bit alignment after asynchronous startup");
    }
    std::cout << "7 test scenarios passed (framing, checksum, bounds, timing, recovery, reset, alignment)\n";
}
