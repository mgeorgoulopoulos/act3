#pragma once
#include <array>
#include <cstdint>

namespace music {
struct Features {
    float rms{}, peak{}, bass{}, mids{}, treble{}, onset{}, pulse{};
    float orchestralPulse{}; // Automatic 350 Hz .. 6 kHz attack envelope.
    std::array<float,64> spectrum{}; // Logarithmic bands, 40 Hz .. 16 kHz.
};
bool load(const char* path);
bool start();
std::uint64_t milliseconds();
const Features& features(); // Live Hann-windowed FFT and automatic spectral-flux pulse.
void release();
}
