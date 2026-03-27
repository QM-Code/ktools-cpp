#include <gamma/sdk.hpp>

#include <ki18n.hpp>

#include <iostream>

namespace {

bool g_initialized = false;

} // namespace

namespace ki18n::demo::gamma {

void Initialize() {
    if (!g_initialized) {
        g_initialized = true;
    }
}

void EmitDemoOutput() {
    Initialize();
    std::cout << "[gamma] demo sdk initialized\\n";
}

} // namespace ki18n::demo::gamma
